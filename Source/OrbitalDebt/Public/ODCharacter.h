#pragma once

#include "CoreMinimal.h"
#include "ODAnimationInterface.h"
#include "ODInteractInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "ODItemData.h"
#include "ODCharacter.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class AODWeapon;
class UODPlayerHUD;

UCLASS()
class ORBITALDEBT_API AODCharacter : public ACharacter, public IODAnimationInterface
{
    GENERATED_BODY()
    
public:
    AODCharacter();
    float NormalWalkSpeed;

protected:
    virtual void BeginPlay() override;
    
    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    USoundBase* JumpSound;

    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    USoundBase* DeathSound;
    
    void PerformJump();

    // --- СМЕНА КАМЕРЫ (ПЛЕЧО) ---
    float DefaultSpringArmY; // Запоминаем стартовое положение
    float TargetSpringArmY;  // Куда двигать камеру
    void ToggleCameraSide(); // Функция смены стороны

    // --- СЛАЙДИНГ (ПОДКАТ) ---
    UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
    UAnimMontage* SlideMontage; // Анимация подката

    UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
    float SlideSpeed = 1200.0f; // Скорость рывка

    UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
    float SlideDuration = 1.0f; // Длительность

    bool bIsSliding;
    FTimerHandle TimerHandle_Slide;
    
    void StartSlide();
    void StopSlide();

public: 
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // --- РЕАЛИЗАЦИЯ ИНТЕРФЕЙСА ---
    virtual bool GetIsAiming_Implementation() const override;
    virtual bool GetIsSprinting_Implementation() const override;
    virtual bool HasWeapon_Implementation() const override;
    // Добавим геттер для АнимБлупринта
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool IsSliding() const { return bIsSliding; }

    // --- АИМ ПИТЧ ---
    virtual float GetAimPitch_Implementation() const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    class USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    class UCameraComponent* FollowCamera;

    // --- INPUT ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* DefaultMappingContext;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* LookAction;
    
    // ОРУЖИЕ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat)
    TSubclassOf<AODWeapon> StartingWeaponClass;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    TArray<UODItemData*> DefaultLoadout;
    
    AODWeapon* GetCurrentWeapon() const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* ShootAction;
    
    void StartWeaponFire(const FInputActionValue& Value);
    void StopWeaponFire(const FInputActionValue& Value);
    
    // АНИМАЦИИ
    void PlayFireAnimation();

    UPROPERTY(EditDefaultsOnly, Category = Animation)
    UAnimMontage* FireMontage;
    UPROPERTY(EditDefaultsOnly, Category = Animation)
    UAnimMontage* AimFireMontage;

    // ПРИЦЕЛИВАНИЕ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* AimAction;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat)
    bool bIsAiming;
    void StartAiming(const FInputActionValue& Value);
    void StopAiming(const FInputActionValue& Value);
    
    // ZOOM
    float DefaultFOV;
    UPROPERTY(EditDefaultsOnly, Category = Combat)
    float AimedFOV = 60.0f;
    UPROPERTY(EditDefaultsOnly, Category = Combat)
    float ZoomInterpSpeed = 20.0f;
    
    // ЗДОРОВЬЕ
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Stats")
    float MaxHealth = 100.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Stats")
    float CurrentHealth;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
    void Die();
    
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UODPlayerHUD> PlayerHUDClass;
    UPROPERTY()
    class UODPlayerHUD* PlayerHUD;
    
    // ПЕРЕЗАРЯДКА / ВЗАИМОДЕЙСТВИЕ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* ReloadAction;
    void Reload(const FInputActionValue& Value);
    void UpdateAmmoHUD();
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* InteractAction;
    void Interact();
    
    // ИНВЕНТАРЬ
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    class UODInventoryComponent* InventoryComponent;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* InventoryAction;
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UODInventoryWindow> InventoryWindowClass;
    UPROPERTY()
    class UODInventoryWindow* InventoryWindow;
    void ToggleInventory();
    UFUNCTION()
    void OnInventoryUpdated();
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* SprintAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* JumpAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* DropAction;

protected:
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    
    UPROPERTY(EditDefaultsOnly, Category = "Interaction")
    float InteractionDistance = 250.0f;
    UPROPERTY(VisibleAnywhere, Category = "Interaction")
    AActor* FocusedActor;
    
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Aiming")
    float AimWalkSpeed = 300.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Aiming", meta = (ClampMin="0.1", ClampMax="1.0"))
    float AimSensitivityMultiplier = 0.5f;
    
    void PerformInteractionCheck();
    
    UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
    float WalkSpeed = 400.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
    float SprintSpeed = 800.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Movement|Jump")
    float JumpVelocity = 600.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Movement|Jump")
    float AirControlAmount = 0.5f;

    bool bIsSprinting;
    void StartSprint(const FInputActionValue& Value);
    void StopSprint(const FInputActionValue& Value);
    
    void DebugDropItem();
};