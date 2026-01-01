#pragma once

#include "CoreMinimal.h"
#include "ODAnimationInterface.h"
#include "ODInteractInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h" // Важно для работы с входными данными
#include "ODItemData.h"
#include "ODCharacter.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class AODWeapon; // Forward declaration, чтобы C++ знал, что такой класс существует
class UODPlayerHUD;

UCLASS()
class ORBITALDEBT_API AODCharacter : public ACharacter, public IODAnimationInterface
{
	GENERATED_BODY()
	
public:
	AODCharacter();
	// Чтобы запомнить обычную скорость и вернуть её потом
	float NormalWalkSpeed;

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* JumpSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* DeathSound;
    
	// Обертка для прыжка, чтобы проиграть звук
	void PerformJump();

public:	
	
	virtual float GetAimPitch_Implementation() const override;
	
	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- РЕАЛИЗАЦИЯ ИНТЕРФЕЙСА ---
	virtual bool GetIsAiming_Implementation() const override;
	virtual bool GetIsSprinting_Implementation() const override;
	virtual bool HasWeapon_Implementation() const override;
	
	// --- КОМПОНЕНТЫ ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class USpringArmComponent* CameraBoom; // Штатив

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UCameraComponent* FollowCamera; // Сама камера

	// --- INPUT (ВХОДНЫЕ ДАННЫЕ) ---
	
	// Контекст управления (раскладка клавиш)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	// Действие: Движение (WASD)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	// Действие: Обзор (Мышь)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;
	
	// --- ОРУЖИЕ ---

	// Класс оружия, с которым мы появимся (выберем BP_Rifle_Base в редакторе)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat)
	TSubclassOf<AODWeapon> StartingWeaponClass;

	// ВМЕСТО StartingWeaponClass делаем список предметов
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<UODItemData*> DefaultLoadout;
	
	AODWeapon* GetCurrentWeapon() const;

	// Действие стрельбы
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ShootAction;

	
	// Функции управления стрельбой
	void StartWeaponFire(const FInputActionValue& Value);
	void StopWeaponFire(const FInputActionValue& Value);
	
	// --- АНИМАЦИИ ---
	
	// Анимации стрельбы
	void PlayFireAnimation();

	// Монтаж выстрела от бедра
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* FireMontage;

	// НОВОЕ: Монтаж выстрела в прицеле
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* AimFireMontage;

	// --- ПРИЦЕЛИВАНИЕ ---

	// Действие прицеливания
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	// Переменная: Целимся ли мы сейчас? (Будет читать AnimBP)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat)
	bool bIsAiming;

	// Функции нажатия/отпускания ПКМ
	void StartAiming(const FInputActionValue& Value);
	void StopAiming(const FInputActionValue& Value);
	
	// --- ZOOM (FOV) ---

	// Угол обзора по умолчанию (запомним при старте)
	float DefaultFOV;

	// Угол обзора при прицеливании (чем меньше число, тем сильнее зум)
	UPROPERTY(EditDefaultsOnly, Category = Combat)
	float AimedFOV = 60.0f;

	// Скорость зума (интерполяция)
	UPROPERTY(EditDefaultsOnly, Category = Combat)
	float ZoomInterpSpeed = 20.0f;
	
	// --- ЗДОРОВЬЕ ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Stats")
	float CurrentHealth;

	// Функция получения урона (стандартная для Unreal)
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// Функция смерти (чтобы не захламлять TakeDamage)
	void Die();
	
	// Класс виджета (чтобы выбрать WBP_PlayerHUD в редакторе)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UODPlayerHUD> PlayerHUDClass;

	// Ссылка на созданный виджет (чтобы вызывать UpdateHealth)
	UPROPERTY()
	class UODPlayerHUD* PlayerHUD;
	
	// --- ПЕРЕЗАРЯДКА ---
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ReloadAction;
	
	void Reload(const FInputActionValue& Value);
	
	// Функция-посредник: Оружие дергает её, а она дергает Виджет
	void UpdateAmmoHUD();
	
	// Инпут для взаимодействия (Кнопка F)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* InteractAction;
	
	// Функция нажатия кнопки
	void Interact();
	
	// --- ИНВЕНТАРЬ ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	class UODInventoryComponent* InventoryComponent;
	
	// --- UI ИНВЕНТАРЯ ---
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* InventoryAction;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UODInventoryWindow> InventoryWindowClass;

	UPROPERTY()
	class UODInventoryWindow* InventoryWindow;

	// Функция переключения
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
	// Функции обработки ввода
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	
	// Как далеко мы можем дотянуться (например, 2-3 метра)
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionDistance = 250.0f;

	// Найденный предмет, на который мы смотрим прямо сейчас
	// Мы храним его как TScriptInterface (умная ссылка на интерфейс)
	// Но для простоты пока используем AActor* и проверку Implements<>
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	AActor* FocusedActor;
	
	// Скорость ходьбы при прицеливании (обычно 300, при беге 600)
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Aiming")
	float AimWalkSpeed = 300.0f;

	// Множитель чувствительности мыши при прицеливании (0.5 = в 2 раза медленнее)
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Aiming", meta = (ClampMin="0.1", ClampMax="1.0"))
	float AimSensitivityMultiplier = 0.5f;
	
	// Функция проверки (будем вызывать в Tick)
	void PerformInteractionCheck();
	
	// --- НАСТРОЙКИ ДВИЖЕНИЯ ---
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
	float WalkSpeed = 400.0f;     // Обычная ходьба

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
	float SprintSpeed = 800.0f;   // Бег с Shift

	// Настройки прыжка
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Jump")
	float JumpVelocity = 600.0f;  // Сила прыжка (высота)

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Jump")
	float AirControlAmount = 0.5f; // Управление в полете (0 = летим как камень, 1 = полный контроль)

	// --- НАСТРОЙКИ ТРЯСКИ (HEAD BOB) ---
	// Мы разделили настройки на "Ходьбу" и "Бег", чтобы ты мог менять их отдельно

	UPROPERTY(EditDefaultsOnly, Category = "Effects|HeadBob")
	float BobFreq_Walk = 10.0f; // Частота шагов (ходьба)

	UPROPERTY(EditDefaultsOnly, Category = "Effects|HeadBob")
	float BobAmp_Walk = 1.0f;   // Сила тряски (ходьба)

	UPROPERTY(EditDefaultsOnly, Category = "Effects|HeadBob")
	float BobFreq_Sprint = 20.0f; // Частота шагов (бег) - быстрее!

	UPROPERTY(EditDefaultsOnly, Category = "Effects|HeadBob")
	float BobAmp_Sprint = 3.5f;   // Сила тряски (бег) - сильнее!

	// Флаг для анимации и логики
	bool bIsSprinting;

	// Функции спринта
	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);
	
	// Временная функция для теста выброса
	void DebugDropItem();
};