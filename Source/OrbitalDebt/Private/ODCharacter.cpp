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
	CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 70.0f);

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
		float TargetFOV = bIsAiming ? AimedFOV : DefaultFOV;
		float CurrentFOV = FollowCamera->FieldOfView;
		float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, ZoomInterpSpeed);
		FollowCamera->SetFieldOfView(NewFOV);
		
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
		
		// Используем DropActiveWeapon вместо старого DebugDrop
		EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &AODCharacter::DebugDropItem);
	}

	// Хотбар 1-8
	PlayerInputComponent->BindAction<TDelegate<void(int32)>>("Hotbar1", IE_Pressed, InventoryComponent, &UODInventoryComponent::SelectHotbarSlot, 0);
	PlayerInputComponent->BindAction<TDelegate<void(int32)>>("Hotbar2", IE_Pressed, InventoryComponent, &UODInventoryComponent::SelectHotbarSlot, 1);
	PlayerInputComponent->BindAction<TDelegate<void(int32)>>("Hotbar3", IE_Pressed, InventoryComponent, &UODInventoryComponent::SelectHotbarSlot, 2);
	// ... добавьте остальные по аналогии
}

void AODCharacter::PerformJump()
{
	Jump();
	if (JumpSound) UGameplayStatics::PlaySoundAtLocation(this, JumpSound, GetActorLocation());
}

void AODCharacter::Move(const FInputActionValue& Value)
{
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
AODWeapon* AODCharacter::GetCurrentWeapon() const { return InventoryComponent ? InventoryComponent->GetCurrentWeapon() : nullptr; }
// AIM Pitch
float AODCharacter::GetAimPitch_Implementation() const
{
	if (!GetController()) return 0.0f;
	FRotator Delta = GetControlRotation() - GetActorRotation();
	Delta.Normalize();
	return Delta.Pitch;
}