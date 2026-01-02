#include "ODCharacter.h"
#include "ODWeapon.h"
#include "GameFramework/SpringArmComponent.h"
#include "ODInventoryComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ODInventoryWindow.h"
#include "ODPlayerHUD.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AODCharacter::AODCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true; 
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); 
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; 
	CameraBoom->bUsePawnControlRotation = true; 
    
	// Ставим начальное смещение
	CameraBoom->SocketOffset = FVector(0.0f, 60.0f, 70.0f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); 
	FollowCamera->bUsePawnControlRotation = false; 

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -97.0f)); 
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetOwnerNoSee(false); 

	InventoryComponent = CreateDefaultSubobject<UODInventoryComponent>(TEXT("PlayerInventory"));
}

void AODCharacter::BeginPlay()
{
	Super::BeginPlay();
    
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	NormalWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	CurrentHealth = MaxHealth;

	// Запоминаем дефолтное положение камеры (Y = 60.0f)
	DefaultSpringArmY = CameraBoom->SocketOffset.Y;
	TargetSpringArmY = DefaultSpringArmY;
    
	if (PlayerHUDClass && IsLocallyControlled())
	{
		PlayerHUD = CreateWidget<UODPlayerHUD>(GetWorld(), PlayerHUDClass);
		if (PlayerHUD)
		{
			PlayerHUD->AddToViewport();
			PlayerHUD->UpdateHealth(CurrentHealth, MaxHealth);
		}
        
		if (InventoryComponent)
		{
			InventoryComponent->OnInventoryUpdated.AddDynamic(this, &AODCharacter::OnInventoryUpdated);
		}
	}
    
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
    
	if (InventoryComponent)
	{
		for (UODItemData* Item : DefaultLoadout)
		{
			InventoryComponent->TryAddItem(Item); 
		}
	}
    
	if (FollowCamera)
	{
		DefaultFOV = FollowCamera->FieldOfView;
	}
}

void AODCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    
	if (FollowCamera)
	{
		// Зум прицеливания
		float TargetFOV = bIsAiming ? AimedFOV : DefaultFOV;
		float CurrentFOV = FollowCamera->FieldOfView;
		float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, ZoomInterpSpeed);
		FollowCamera->SetFieldOfView(NewFOV);
        
		// Плавная смена плеча камеры
		float CurrentY = CameraBoom->SocketOffset.Y;
		if (!FMath::IsNearlyEqual(CurrentY, TargetSpringArmY, 0.1f))
		{
			float NewY = FMath::FInterpTo(CurrentY, TargetSpringArmY, DeltaTime, 10.0f);
			CameraBoom->SocketOffset.Y = NewY;
		}

		PerformInteractionCheck();
	}
}

void AODCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AODCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AODCharacter::Look);
        EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AODCharacter::StartWeaponFire);
        EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &AODCharacter::StopWeaponFire);
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AODCharacter::StartAiming);
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AODCharacter::StopAiming);
        EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AODCharacter::Reload);
        EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AODCharacter::Interact);
        EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AODCharacter::ToggleInventory);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AODCharacter::PerformJump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AODCharacter::StartSprint);
        EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AODCharacter::StopSprint);
        EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &AODCharacter::DebugDropItem);
    }

    // Биндинг клавиш 1-8 для смены оружия (Проверьте Project Settings -> Input -> Action Mappings!)
    PlayerInputComponent->BindAction<TDelegate<void(int32)>>("Hotbar1", IE_Pressed, InventoryComponent, &UODInventoryComponent::SelectHotbarSlot, 0);
    PlayerInputComponent->BindAction<TDelegate<void(int32)>>("Hotbar2", IE_Pressed, InventoryComponent, &UODInventoryComponent::SelectHotbarSlot, 1);
    PlayerInputComponent->BindAction<TDelegate<void(int32)>>("Hotbar3", IE_Pressed, InventoryComponent, &UODInventoryComponent::SelectHotbarSlot, 2);
    PlayerInputComponent->BindAction<TDelegate<void(int32)>>("Hotbar4", IE_Pressed, InventoryComponent, &UODInventoryComponent::SelectHotbarSlot, 3);

    // Смена камеры на X
    PlayerInputComponent->BindAction("CameraSwap", IE_Pressed, this, &AODCharacter::ToggleCameraSide);
    
    // Слайдинг на Alt
    PlayerInputComponent->BindAction("Slide", IE_Pressed, this, &AODCharacter::StartSlide);
}

void AODCharacter::ToggleCameraSide()
{
	// Если сейчас справа (+), ставим влево (-), и наоборот
	if (TargetSpringArmY > 0) TargetSpringArmY = -DefaultSpringArmY;
	else TargetSpringArmY = DefaultSpringArmY;
}

void AODCharacter::StartSlide()
{
	// Слайдим, только если бежим, на земле и не в откате
	if (bIsSprinting && GetCharacterMovement()->IsMovingOnGround() && !bIsSliding)
	{
		bIsSliding = true;
        
		// Уменьшаем трение, чтобы скользить
		GetCharacterMovement()->GroundFriction = 0.0f;
		GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;
        
		// Даем импульс вперед (или используем текущую скорость)
		FVector SlideDirection = GetActorForwardVector();
		GetCharacterMovement()->Velocity = SlideDirection * SlideSpeed;

		// Играем анимацию (Монтаж должен быть создан в редакторе)
		if (SlideMontage)
		{
			PlayAnimMontage(SlideMontage);
		}
        
		// Уменьшаем капсулу (чтобы проезжать под препятствиями)
		GetCapsuleComponent()->SetCapsuleHalfHeight(48.0f); 

		// Таймер остановки
		GetWorldTimerManager().SetTimer(TimerHandle_Slide, this, &AODCharacter::StopSlide, SlideDuration, false);
	}
}

void AODCharacter::StopSlide()
{
	bIsSliding = false;
    
	// Возвращаем физику ходьбы
	GetCharacterMovement()->GroundFriction = 8.0f; // Стандартное значение
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f; 
    
	// Возвращаем капсулу
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
    
	// Если все еще держим Shift - бежим, иначе идем
	if (bIsSprinting) GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	else GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AODCharacter::PerformJump()
{
	if (bIsSliding) return; // Не прыгать в подкате
	Jump();
	if (JumpSound) UGameplayStatics::PlaySoundAtLocation(this, JumpSound, GetActorLocation());
}

void AODCharacter::Move(const FInputActionValue& Value)
{
	if (bIsSliding) return; // Не управляем движением во время подката
    
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AODCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (bIsAiming) LookAxisVector *= AimSensitivityMultiplier;
	
	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AODCharacter::PlayFireAnimation()
{
	UAnimMontage* MontageToPlay = bIsAiming ? AimFireMontage : FireMontage;
	if (MontageToPlay)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance) AnimInstance->Montage_Play(MontageToPlay, 1.0f);
	}
}

void AODCharacter::StartAiming(const FInputActionValue& Value)
{
	bIsAiming = true;
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed; 
}

void AODCharacter::StopAiming(const FInputActionValue& Value)
{
	bIsAiming = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
}

float AODCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	if (PlayerHUD) PlayerHUD->UpdateHealth(CurrentHealth, MaxHealth);
	if (CurrentHealth <= 0.0f) Die();
	return ActualDamage;
}

void AODCharacter::Die()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController())) DisableInput(PC);
	if (DeathSound) UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AODCharacter::Reload(const FInputActionValue& Value)
{
	if (InventoryComponent->GetCurrentWeapon() && InventoryComponent->GetCurrentWeapon()->CurrentAmmo < InventoryComponent->GetCurrentWeapon()->MagCapacity)
	{
		InventoryComponent->GetCurrentWeapon()->Reload();
		if (InventoryComponent->GetCurrentWeapon()->ReloadMontage)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance) AnimInstance->Montage_Play(InventoryComponent->GetCurrentWeapon()->ReloadMontage);
		}
	}
}

void AODCharacter::StartWeaponFire(const FInputActionValue& Value)
{
	if (InventoryComponent->GetCurrentWeapon()) InventoryComponent->GetCurrentWeapon()->StartFire();
}

void AODCharacter::StopWeaponFire(const FInputActionValue& Value)
{
	if (InventoryComponent->GetCurrentWeapon()) InventoryComponent->GetCurrentWeapon()->StopFire();
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
	if (!FollowCamera || !CameraBoom) return;

	FVector Start = FollowCamera->GetComponentLocation();
	float CombinedDistance = CameraBoom->TargetArmLength + InteractionDistance;
	FVector End = Start + (FollowCamera->GetForwardVector() * CombinedDistance);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (GetCurrentWeapon()) Params.AddIgnoredActor(GetCurrentWeapon());

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor == FocusedActor) return; 

		if (HitActor->Implements<UODInteractInterface>())
		{
			FocusedActor = HitActor;
			if (PlayerHUD)
			{
				FText Message = IODInteractInterface::Execute_GetInteractText(HitActor);
				PlayerHUD->SetInteractionPrompt(true, Message);
			}
			return; 
		}
	}

	if (FocusedActor)
	{
		FocusedActor = nullptr;
		if (PlayerHUD) PlayerHUD->SetInteractionPrompt(false);
	}
}

void AODCharacter::Interact()
{
	if (FocusedActor) IODInteractInterface::Execute_Interact(FocusedActor, this);
}

void AODCharacter::ToggleInventory()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !InventoryWindowClass) return;

	if (!InventoryWindow) InventoryWindow = CreateWidget<UODInventoryWindow>(GetWorld(), InventoryWindowClass);

	if (InventoryWindow)
	{
		if (InventoryWindow->IsInViewport())
		{
			InventoryWindow->RemoveFromParent();
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
		else
		{
			InventoryWindow->AddToViewport();
			if (InventoryComponent) InventoryWindow->RefreshInventory(InventoryComponent->GetItems());
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(InventoryWindow->TakeWidget());
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
	}
}

void AODCharacter::OnInventoryUpdated()
{
	if (InventoryWindow && InventoryWindow->IsInViewport() && InventoryComponent)
	{
		InventoryWindow->RefreshInventory(InventoryComponent->GetItems());
	}
}

void AODCharacter::StartSprint(const FInputActionValue& Value)
{
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
		// Если есть оружие в руках - выбрасываем его
		if (InventoryComponent->GetCurrentWeapon())
		{
			InventoryComponent->DropActiveWeapon();
		}
		// Иначе выбрасываем первый предмет из рюкзака
		else if (InventoryComponent->GetItems().Num() > 0)
		{
			InventoryComponent->DropItem(0);
		}
	}
}

// Реализация интерфейса
bool AODCharacter::GetIsAiming_Implementation() const { return bIsAiming; }
bool AODCharacter::GetIsSprinting_Implementation() const { return bIsSprinting; }
bool AODCharacter::HasWeapon_Implementation() const { return (GetCurrentWeapon() != nullptr); }

// Геттер оружия
AODWeapon* AODCharacter::GetCurrentWeapon() const { 
	return InventoryComponent ? InventoryComponent->GetCurrentWeapon() : nullptr; 
}

// AIM Pitch
float AODCharacter::GetAimPitch_Implementation() const
{
	if (!GetController()) return 0.0f;
	FRotator Delta = GetControlRotation() - GetActorRotation();
	Delta.Normalize();
	return Delta.Pitch;
}