#include "ODWeapon.h"
#include "Kismet/GameplayStatics.h"
#include "ODEnemy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "DrawDebugHelpers.h" // Нужен для рисования линий (дебаг)

AODWeapon::AODWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Создаем пустую точку (SceneComponent) и делаем её КОРНЕМ
	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot; // <--- Теперь это главный компонент

	// 2. Создаем Меш
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    
	// 3. Прикрепляем Меш к новому Корню (делаем дочерним)
	WeaponMesh->SetupAttachment(DefaultSceneRoot); 

	// 4. Создаем стрелочку и крепим к Мешу (как и было раньше)
	MuzzleArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("MuzzleArrow"));
	MuzzleArrow->SetupAttachment(WeaponMesh);
}

void AODWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentAmmo = MagCapacity; // Полный магазин на старте
	bIsReloading = false;
	
	// Вычисляем время между выстрелами. 
	// Если 600 выстр/мин, то 60/600 = 0.1 сек между пулями.
	TimeBetweenShots = 60.0f / RateOfFire;
}

void AODWeapon::Fire()
{
    // Проверки
    if (!MuzzleArrow || bIsReloading || !CanFire()) 
    {
        StopFire();
        return;
    };
    
    // Анимации и тряска (оставляем как было)
    if (AActor* OwnerActor = GetOwner())
    {
        if (AODCharacter* Player = Cast<AODCharacter>(OwnerActor)) Player->PlayFireAnimation();
        else if (AODEnemy* Enemy = Cast<AODEnemy>(OwnerActor)) Enemy->PlayFireAnimation();
    }
    
    if (FireCameraShake)
    {
        if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
            if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
                PC->ClientStartCameraShake(FireCameraShake);
    }
    
    // ИГРАЕМ ЗВУК
    if (FireSound) UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
    
    // --- НОВАЯ ЛОГИКА ТРЕЙСА (TPS) ---
    FVector TraceStart;
    FVector TraceEnd;
    FVector ShotDirection;

    // 1. Пытаемся получить камеру игрока (Стреляем из Глаз)
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

    // Если это Игрок с контроллером
    if (OwnerController && OwnerController->IsLocalPlayerController())
    {
        FVector CamLoc;
        FRotator CamRot;
        OwnerController->GetPlayerViewPoint(CamLoc, CamRot); // Получаем позицию камеры
            
        TraceStart = CamLoc;
        ShotDirection = CamRot.Vector();
        TraceEnd = TraceStart + (ShotDirection * FireRange);
    }
    // Если это Бот (у него нет камеры) -> Стреляем по старинке из Дула
    else 
    {
        TraceStart = MuzzleArrow->GetComponentLocation();
        ShotDirection = MuzzleArrow->GetForwardVector();
        TraceEnd = TraceStart + (ShotDirection * FireRange);
    }

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    if (GetOwner()) QueryParams.AddIgnoredActor(GetOwner());

    FHitResult Hit;
    bool bHasHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

    // --- ВИЗУАЛ (ОТ ДУЛА) ---
    // Пуля летит от оружия к точке, куда смотрела камера
    if (MuzzleArrow)
    {
        FVector MuzzleLocation = MuzzleArrow->GetComponentLocation();
        FVector ImpactPoint = bHasHit ? Hit.ImpactPoint : TraceEnd;
        
        // Рисуем линию от ствола
        DrawDebugLine(GetWorld(), MuzzleLocation, ImpactPoint, FColor::Red, false, 1.0f, 0, 1.0f);
    }

    // --- УРОН ---
    if (bHasHit && Hit.GetActor())
    {
        UParticleSystem* SelectedVFX = DefaultImpactVFX; 
        if (Hit.GetActor()->IsA(AODEnemy::StaticClass()) && EnemyImpactVFX) SelectedVFX = EnemyImpactVFX;

        if (SelectedVFX) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedVFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());

        UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage, ShotDirection, Hit, GetInstigatorController(), this, UDamageType::StaticClass());
    }
    
    // Расход патронов
    if (!bInfiniteAmmo)
    {
        CurrentAmmo--;
        if (AActor* OwnerActor = GetOwner())
            if (AODCharacter* Player = Cast<AODCharacter>(OwnerActor)) Player->UpdateAmmoHUD();
    }
}
bool AODWeapon::CanFire() const
{
	return CurrentAmmo > 0;
}

void AODWeapon::Reload()
{
	// Если уже полный или уже перезаряжаемся - не надо
	if (CurrentAmmo >= MagCapacity || bIsReloading) return;
	
	// ИГРАЕМ ЗВУК ПЕРЕЗАРЯДКИ
	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}

	UE_LOG(LogTemp, Warning, TEXT("Reloading..."));
	bIsReloading = true;

	// Расчет времени перезарядки
	float ReloadDuration = 2.0f; // Дефолт, если нет анимации
	
	// ВАЖНЫЙ МОМЕНТ:
	// Оружие само не знает, кто его держит (Игрок или Бот).
	// Поэтому мы просто запускаем таймер логики. 
	// А анимацию проиграет Владелец (Character) отдельно.
	
	if (ReloadMontage)
	{
		// Берем длительность из монтажа, если он есть
		ReloadDuration = ReloadMontage->GetPlayLength();
	}

	// Запускаем таймер, который вызовет FinishReload через ReloadDuration секунд
	GetWorldTimerManager().SetTimer(TimerHandle_Reload, this, &AODWeapon::FinishReload, ReloadDuration, false);
}

void AODWeapon::FinishReload()
{
	bIsReloading = false;
	CurrentAmmo = MagCapacity;
	
	// --- ОБНОВЛЯЕМ HUD ИГРОКА ---
	if (AActor* OwnerActor = GetOwner())
	{
		if (AODCharacter* Player = Cast<AODCharacter>(OwnerActor))
		{
			Player->UpdateAmmoHUD();
		}
	}
}

void AODWeapon::StartFire()
{
	// Первый выстрел делаем мгновенно (чтобы не было задержки при нажатии)
	// Но только если мы не перезаряжаемся и есть патроны
	if (!bIsReloading && CanFire())
	{
		// Делаем выстрел
		Fire(); 

		// Запускаем таймер, который будет вызывать Fire() каждые TimeBetweenShots секунд
		// true = повторяющийся таймер (Loop)
		GetWorldTimerManager().SetTimer(TimerHandle_AutoFire, this, &AODWeapon::Fire, TimeBetweenShots, true);
		
	}else if (!bIsReloading && CurrentAmmo <= 0)
	{
		// Если патронов нет — щелкаем
		if (DryFireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, DryFireSound, GetActorLocation());
		}
	}
}

void AODWeapon::StopFire()
{
	// Просто останавливаем таймер
	GetWorldTimerManager().ClearTimer(TimerHandle_AutoFire);
}