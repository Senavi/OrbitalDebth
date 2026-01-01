#include "ODWeapon.h"
#include "ODCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "ODInventoryComponent.h" // Не забудь инклюд!
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ODInventoryWindow.h"
#include "ODPlayerHUD.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// ODCharacter.cpp

AODCharacter::AODCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. Настраиваем капсулу
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // 2. ОТКЛЮЧАЕМ вращение персонажа вслед за камерой
    // (Чтобы мы могли осмотреться вокруг, не поворачивая героя)
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 3. Настраиваем движение
    // Персонаж будет поворачиваться в ту сторону, куда идет
    GetCharacterMovement()->bOrientRotationToMovement = true; 
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // Скорость поворота

    GetCharacterMovement()->JumpZVelocity = 700.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

    // 4. Создаем ШТАТИВ (Spring Arm)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 300.0f; // Дистанция камеры (3 метра)
    CameraBoom->bUsePawnControlRotation = true; // Штатив вращается за мышкой
    
    // Сдвигаем камеру чуть вправо и вверх (плечо)
    CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 70.0f);

    // 5. Создаем КАМЕРУ
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Крепим к концу штатива
    FollowCamera->bUsePawnControlRotation = false; // Камера просто висит на штативе

    // 6. Настраиваем ТЕЛО (Mesh)
    // ВАЖНО: Крепим к Капсуле (не к камере!)
    // Опускаем вниз и поворачиваем
    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -97.0f)); 
    GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    
    // Включаем видимость (игрок должен видеть себя)
    GetMesh()->SetOwnerNoSee(false); 

    // Создаем инвентарь (как и было)
    InventoryComponent = CreateDefaultSubobject<UODInventoryComponent>(TEXT("PlayerInventory"));
}

void AODCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Применяем настройки движения при старте
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->JumpZVelocity = JumpVelocity;
	GetCharacterMovement()->AirControl = AirControlAmount;
	
	// Запоминаем скорость, настроенную в CharacterMovement (например, 600)
	NormalWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	
	// Полное здоровье при старте
	CurrentHealth = MaxHealth;
	
	// --- UI SETUP ---
	// Проверяем, что класс выбран в блюпринте и мы управляем персонажем локально (IsLocallyControlled)
	if (PlayerHUDClass && IsLocallyControlled())
	{
		// Создаем виджет
		PlayerHUD = CreateWidget<UODPlayerHUD>(GetWorld(), PlayerHUDClass);
		
		if (PlayerHUD)
		{
			// Добавляем на экран
			PlayerHUD->AddToViewport();
			
			// Сразу обновляем (чтобы показать 100%)
			PlayerHUD->UpdateHealth(CurrentHealth, MaxHealth);
		}
		
		// Подписываемся на обновление инвентаря
		if (InventoryComponent)
		{
			InventoryComponent->OnInventoryUpdated.AddDynamic(this, &AODCharacter::OnInventoryUpdated);
		}
	}
	
	if (PlayerHUD)
	{
		PlayerHUD->AddToViewport();
		PlayerHUD->UpdateHealth(CurrentHealth, MaxHealth);
		
		// НОВОЕ: Обновляем патроны при старте
		UpdateAmmoHUD(); 
	}

	// Добавляем Mapping Context (раскладку) игроку при старте
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Приоритет 0 - базовый
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// ВЫДАЧА СТАРТОВОГО СНАРЯЖЕНИЯ
	if (InventoryComponent)
	{
		for (UODItemData* Item : DefaultLoadout)
		{
			InventoryComponent->TryAddItem(Item); // Инвентарь сам решит: надеть или в рюкзак
		}
	}
	
	// Запоминаем исходный FOV камеры (обычно 90 градусов)
	if (FollowCamera)
	{
		DefaultFOV = FollowCamera->FieldOfView;
	}
}

void AODCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Логика Зума (FOV)
	if (FollowCamera)
	{
		// Целевой угол: Если целимся -> берем AimedFOV, иначе -> DefaultFOV
		float TargetFOV = bIsAiming ? AimedFOV : DefaultFOV;

		// Текущий угол
		float CurrentFOV = FollowCamera->FieldOfView;

		// Плавное изменение (FInterpTo) от Текущего к Целевому
		float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, ZoomInterpSpeed);

		// Применяем новый угол камере
		FollowCamera->SetFieldOfView(NewFOV);
		
		// Проверяем, на что смотрим
		PerformInteractionCheck();
	}
}

void AODCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Мы не вызываем Super, потому что настраиваем свой Enhanced Input
	
	// Приводим стандартный компонент к EnhancedInputComponent
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Привязываем функцию движения (Triggered вызывается каждый кадр пока кнопка нажата)
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AODCharacter::Move);

		// Привязываем функцию обзора
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AODCharacter::Look);
		
		// Привязываем стрельбу (Started = один раз при нажатии)
		// Стрельба (FireAction)
		// Started = Нажал кнопку -> Начинаем поливать огнем
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AODCharacter::StartWeaponFire);
	
		// Completed = Отпустил кнопку -> Прекращаем огонь
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &AODCharacter::StopWeaponFire);
		
		// Прицеливание (Started = нажали, Completed = отпустили)
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AODCharacter::StartAiming);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AODCharacter::StopAiming);
		
		// Перезарядка
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AODCharacter::Reload);
		
		// InteractAction создай в редакторе позже и назначь на F
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AODCharacter::Interact);
		EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AODCharacter::ToggleInventory);
		
		// ПРЫЖОК (Используем встроенные функции ACharacter)
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AODCharacter::PerformJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// СПРИНТ
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AODCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AODCharacter::StopSprint);
		
		// В SetupPlayerInputComponent добавь (предварительно создав Action IA_Drop):
		EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &AODCharacter::DebugDropItem);
	}
}

void AODCharacter::PerformJump()
{
	// Вызываем стандартный прыжок
	Jump();

	// Играем звук
	if (JumpSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, JumpSound, GetActorLocation());
	}
}

void AODCharacter::Move(const FInputActionValue& Value)
{
	// Получаем вектор (X, Y) с клавиатуры
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Выясняем, куда смотрит камера (нам нужен только поворот по YAW - вокруг вертикальной оси)
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Получаем направления "Вперед" и "Вправо" относительно поворота персонажа
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Добавляем движение
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AODCharacter::Look(const FInputActionValue& Value)
{
	// Получаем дельту мыши (X, Y)
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// 2. Если целимся - уменьшаем их
	if (bIsAiming)
	{
		LookAxisVector *= AimSensitivityMultiplier;
	}
	
	if (Controller != nullptr)
	{
		// Вращаем контроллер (камеру)
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AODCharacter::PlayFireAnimation()
{
	// Выбираем монтаж: если bIsAiming (целимся) -> AimFireMontage, иначе -> FireMontage
	UAnimMontage* MontageToPlay = bIsAiming ? AimFireMontage : FireMontage;

	if (MontageToPlay)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(MontageToPlay, 1.0f);
		}
	}
}

void AODCharacter::StartAiming(const FInputActionValue& Value)
{
	bIsAiming = true;
    
	// --- НОВАЯ ЛОГИКА ---
	// 1. Заставляем персонажа поворачиваться туда, куда смотрит камера
	bUseControllerRotationYaw = true;

	// 2. Отключаем поворот "в сторону движения" (чтобы можно было ходить боком/стрейфить)
	GetCharacterMovement()->bOrientRotationToMovement = false;
    
	// 3. Замедляем скорость (опционально, если хочешь ходить медленнее в прицеле)
	GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed; 
}

void AODCharacter::StopAiming(const FInputActionValue& Value)
{
	bIsAiming = false;
    
	// --- НОВАЯ ЛОГИКА ---
	// 1. Отцепляем персонажа от камеры (свободный обзор)
	bUseControllerRotationYaw = false;

	// 2. Включаем поворот "в сторону движения" (чтобы бегать свободно)
	GetCharacterMovement()->bOrientRotationToMovement = true;
    
	// 3. Возвращаем обычную скорость
	GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
}

float AODCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// Вычитаем здоровье
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

	// Пишем в лог (чтобы ты видел, что тебе больно)
	UE_LOG(LogTemp, Warning, TEXT("OUCH! Player Health: %f"), CurrentHealth);

	// --- ОБНОВЛЕНИЕ UI ---
	if (PlayerHUD)
	{
		PlayerHUD->UpdateHealth(CurrentHealth, MaxHealth);
	}
	
	// Если здоровье кончилось - умираем
	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return ActualDamage;
}

void AODCharacter::Die()
{
	UE_LOG(LogTemp, Error, TEXT("=== PLAYER DIED ==="));

	// 1. Отключаем управление (чтобы нельзя было бегать мертвым)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
	
	// Крик смерти
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// 2. Включаем физику (Ragdoll), чтобы упасть как мешок
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	
	// 3. Отключаем капсулу (чтобы она не мешала падать)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// (Позже здесь добавим перезагрузку уровня через 3 секунды)
}

void AODCharacter::Reload(const FInputActionValue& Value)
{
	if (InventoryComponent->GetCurrentWeapon())
	{
		// 1. Логика оружия (запустить таймер и флаг)
		InventoryComponent->GetCurrentWeapon()->Reload();

		// 2. Визуал (проиграть анимацию рук)
		// Убедись, что в оружии выбран правильный монтаж (Reload_Rifle)
		if (InventoryComponent->GetCurrentWeapon()->ReloadMontage)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				AnimInstance->Montage_Play(InventoryComponent->GetCurrentWeapon()->ReloadMontage);
			}
		}
	}
}

void AODCharacter::StartWeaponFire(const FInputActionValue& Value)
{
	if (InventoryComponent->GetCurrentWeapon())
	{
		InventoryComponent->GetCurrentWeapon()->StartFire();
	}
}

void AODCharacter::StopWeaponFire(const FInputActionValue& Value)
{
	if (InventoryComponent->GetCurrentWeapon())
	{
		InventoryComponent->GetCurrentWeapon()->StopFire();
	}
}

void AODCharacter::UpdateAmmoHUD()
{
	if (PlayerHUD && InventoryComponent->GetCurrentWeapon())
	{
		PlayerHUD->UpdateAmmo(InventoryComponent->GetCurrentWeapon()->CurrentAmmo, InventoryComponent->GetCurrentWeapon()->MagCapacity);
	}
}

void AODCharacter::PerformInteractionCheck()
{
	if (!FollowCamera) return;

	// ... (Код расчета Start и End трейса оставляем как был) ...
	FVector Start = FollowCamera->GetComponentLocation();
	FVector End = Start + (FollowCamera->GetForwardVector() * InteractionDistance);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (InventoryComponent->GetCurrentWeapon()) Params.AddIgnoredActor(InventoryComponent->GetCurrentWeapon());

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	// Логика обновления HUD
	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();

		// Если это ТОТ ЖЕ актер, на которого мы уже смотрим - ничего не делаем
		if (HitActor == FocusedActor) 
		{
			return; 
		}

		// Если это НОВЫЙ актер и он Интерактивный
		if (HitActor->Implements<UODInteractInterface>())
		{
			FocusedActor = HitActor;
			
			// ПОКАЗЫВАЕМ ТЕКСТ
			if (PlayerHUD)
			{
				// Получаем текст из интерфейса предмета
				FText Message = IODInteractInterface::Execute_GetInteractText(HitActor);
				PlayerHUD->SetInteractionPrompt(true, Message);
			}
			return; // Выходим
		}
	}

	// ЕСЛИ МЫ ЗДЕСЬ - значит мы смотрим в пустоту или на скучную стену
	
	// Если раньше мы на что-то смотрели, а теперь нет -> Очищаем
	if (FocusedActor)
	{
		FocusedActor = nullptr;
		
		// ПРЯЧЕМ ТЕКСТ
		if (PlayerHUD)
		{
			PlayerHUD->SetInteractionPrompt(false);
		}
	}
}

void AODCharacter::Interact()
{
	if (FocusedActor)
	{
		// Вызываем функцию интерфейса у объекта
		// Execute_Interact - это специальный метод Unreal для вызова интерфейсов (работает и с C++, и с Blueprint)
		IODInteractInterface::Execute_Interact(FocusedActor, this);
	}
}

void AODCharacter::ToggleInventory()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !InventoryWindowClass) return;

	// Если окна еще нет - создаем
	if (!InventoryWindow)
	{
		InventoryWindow = CreateWidget<UODInventoryWindow>(GetWorld(), InventoryWindowClass);
	}

	if (InventoryWindow)
	{
		if (InventoryWindow->IsInViewport())
		{
			// ЗАКРЫВАЕМ
			InventoryWindow->RemoveFromParent();

			// Возвращаем управление в игру
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
		else
		{
			// ОТКРЫВАЕМ
			InventoryWindow->AddToViewport();
			
			// Обновляем данные перед показом
			if (InventoryComponent)
			{
				InventoryWindow->RefreshInventory(InventoryComponent->GetItems());
			}

			// Включаем мышку
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(InventoryWindow->TakeWidget());
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
	}
}

void AODCharacter::OnInventoryUpdated()
{
	// Если окно открыто - обновляем его
	if (InventoryWindow && InventoryWindow->IsInViewport() && InventoryComponent)
	{
		InventoryWindow->RefreshInventory(InventoryComponent->GetItems());
	}
}

void AODCharacter::StartSprint(const FInputActionValue& Value)
{
	// Нельзя бежать, если целимся (опционально)
	if (bIsAiming) return;

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AODCharacter::StopSprint(const FInputActionValue& Value)
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AODCharacter::DebugDropItem()
{
	if (InventoryComponent)
	{
		// 1. Если в руках есть оружие - выбрасываем ЕГО
		if (InventoryComponent->GetPrimaryWeapon().IsValid())
		{
			// EEquipmentSlot::Primary - это слот основного оружия
			InventoryComponent->DropEquippedItem(EEquipmentSlot::Primary);
		}
		// 2. Если рук пустые, но есть что-то в рюкзаке - выбрасываем первый предмет
		else if (InventoryComponent->GetItems().Num() > 0)
		{
			InventoryComponent->DropItem(0);
		}
	}
}

bool AODCharacter::GetIsAiming_Implementation() const
{
	return bIsAiming;
}

bool AODCharacter::GetIsSprinting_Implementation() const
{
	// Проверяем, нажата ли кнопка И движемся ли мы вперед
	// (Обычно переменная bIsSprinting у тебя уже есть, если нет - добавь в .h)
	return bIsSprinting; 
}

bool AODCharacter::HasWeapon_Implementation() const
{
	return (GetCurrentWeapon() != nullptr);
}