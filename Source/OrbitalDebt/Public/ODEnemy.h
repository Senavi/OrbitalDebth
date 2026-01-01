#pragma once

#include "CoreMinimal.h"
#include "ODAnimationInterface.h"
#include "GameFramework/Character.h"
#include "ODEnemy.generated.h"

UCLASS()
class ORBITALDEBT_API AODEnemy : public ACharacter, public IODAnimationInterface
{
	GENERATED_BODY()

public:
	AODEnemy();

protected:
	virtual void BeginPlay() override;
	
	// Лут, который выпадет при смерти
	UPROPERTY(EditAnywhere, Category = "Loot")
	TArray<class UODItemData*> LootTable;

	// --- НОВОЕ: Функция смерти ---
	void Die();
	
	// Тоже меняем на Loadout
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TArray<UODItemData*> DefaultLoadout;
	
	// Компонент инвентаря (Врагу он тоже нужен!)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	class UODInventoryComponent* InventoryComponent;

public:	
	// --- ХАРАКТЕРИСТИКИ ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	// --- ФУНКЦИИ ---

	// Стандартная функция движка для получения урона
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	
	// --- БОЕВАЯ СИСТЕМА ---

	// Монтаж выстрела (назначим AM_Fire в редакторе)
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* FireMontage;

	// Таймер для остановки стрельбы (чтобы стрелять очередями, а не бесконечно)
	FTimerHandle TimerHandle_BurstStop;

	// Функции, которые вызывает Оружие или AI
	void PlayFireAnimation();
	void Reload();
	void StopFire();
	
	// --- ОРУЖИЕ ---

	// Функция атаки (которую будет вызывать AI)
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Attack();
	
	// Состояние боя для анимации
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI State")
	bool bIsAiming;

	// --- РЕАЛИЗАЦИЯ ИНТЕРФЕЙСА ---
	virtual bool GetIsAiming_Implementation() const override;
	virtual bool GetIsSprinting_Implementation() const override;
	virtual bool HasWeapon_Implementation() const override;
};