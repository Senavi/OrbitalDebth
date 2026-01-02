#pragma once

#include "ODCharacter.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ODWeapon.generated.h"

class USkeletalMeshComponent;
class UArrowComponent;

UCLASS()
class ORBITALDEBT_API AODWeapon : public AActor
{
	GENERATED_BODY()
	
private:
	FTimerHandle TimerHandle_Reload;
	
public:	
	AODWeapon();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat|VFX")
	UMaterialInterface* BulletHoleDecal;
	
	// Эффект попадания по стенам (бетон, металл) - ПЕРЕИМЕНУЙ старый ImpactVFX в DefaultImpactVFX
	UPROPERTY(EditDefaultsOnly, Category = "Combat|VFX")
	UParticleSystem* DefaultImpactVFX;

	// НОВОЕ: Эффект попадания по врагам (Кровь)
	UPROPERTY(EditDefaultsOnly, Category = "Combat|VFX")
	UParticleSystem* EnemyImpactVFX;
	
	// Звук выстрела (WAV или SoundCue)
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Sound")
	USoundBase* FireSound;

	// Звук перезарядки
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Sound")
	USoundBase* ReloadSound;

	// Звук "Щелчка" (когда нет патронов)
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Sound")
	USoundBase* DryFireSound;
	

public:	
	// --- НОВОЕ: Это будет наш новый корень ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultSceneRoot;

	// --- СТАРОЕ: Оставляем, но он перестанет быть корнем ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* WeaponMesh;

	// Точка вылета пули (дуло)
	// Используем ArrowComponent, так как его стрелочку удобно видеть в редакторе
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* MuzzleArrow;

	// Дистанция стрельбы (в сантиметрах). 10000 см = 100 метров.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float FireRange = 10000.0f;

	// Урон
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float Damage = 20.0f;
	
	// Эффект попадания (Искры, дым)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Visuals")
	UParticleSystem* ImpactVFX;

	// Основная функция стрельбы
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Fire();
	
	// --- БОЕПРИПАСЫ ---

	// Патронов в магазине сейчас
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 CurrentAmmo;

	// Вместимость магазина (например, 30)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	int32 MagCapacity = 30;

	// Бесконечные ли патроны? (Для отладки или для врагов, если хочешь)
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	bool bInfiniteAmmo = false;

	// --- ПЕРЕЗАРЯДКА ---

	// Анимация перезарядки (Для персонажа - от 1-го лица, для бота - от 3-го)
	// Враг и Игрок будут использовать этот монтаж по-разному
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* ReloadMontage;

	// Функция: Можно ли стрелять? (Есть ли патроны)
	bool CanFire() const;

	// Функция: Начать перезарядку
	void Reload();

	// Функция: Завершить перезарядку (вызовется через таймер или анимацию)
	UFUNCTION(BlueprintCallable)
	void FinishReload();

	// Состояние: перезаряжаемся ли мы прямо сейчас?
	bool bIsReloading;
	
	// --- СКОРОСТРЕЛЬНОСТЬ ---

	// Выстрелов в минуту (RPM). Например, 600 для АК-47.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float RateOfFire = 600.0f;

	// Таймер для автоматической стрельбы
	FTimerHandle TimerHandle_AutoFire;

	// Время между выстрелами (вычислим сами: 60 / RateOfFire)
	float TimeBetweenShots;
	
	// Функции для начала и конца стрельбы (их будет вызывать Персонаж)
	void StartFire();
	void StopFire();
	
	// Класс тряски при выстреле (выберем в блюпринте)
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<UCameraShakeBase> FireCameraShake;
};