#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ODItemData.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	General     UMETA(DisplayName = "General"), // Обычный мусор/ресурсы
	Weapon      UMETA(DisplayName = "Weapon"),
	Armor       UMETA(DisplayName = "Armor"),
	Consumable  UMETA(DisplayName = "Consumable") // Аптечки/Еда
};

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None        UMETA(DisplayName = "None"),
	Primary     UMETA(DisplayName = "Primary Weapon"),
	Secondary   UMETA(DisplayName = "Secondary Weapon"),
	Body        UMETA(DisplayName = "Body Armor"),
	Head        UMETA(DisplayName = "Helmet"),
	Rig         UMETA(DisplayName = "Rig/Pockets") // Карманы
};

/**
 * Паспорт предмета. Хранит только статические данные.
 */
UCLASS()
class ORBITALDEBT_API UODItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- ОСНОВНОЕ ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Data")
	FText ItemName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Data", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Data")
	EItemType ItemType;

	// --- ИНВЕНТАРЬ (СЕТКА) ---

	// Иконка для UI (Сюда перетащим картинку .png)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory UI")
	UTexture2D* Icon;

	// Ширина в клетках (1x1, 2x3 и т.д.)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory UI")
	int32 Width = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory UI")
	int32 Height = 1;

	// Вес (для хардкора)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float Weight = 0.5f;

	// --- 3D МИР ---

	// Какой актер спаунить, если выбросим предмет из инвентаря на землю
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TSubclassOf<class AActor> ItemClass;
	
	// Меш, который будет у предмета, когда он лежит на земле
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	UStaticMesh* PickupMesh;
	
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	EEquipmentSlot EquipSlotType; // В какой слот это можно надеть?
	
	// НОВОЕ: Класс реального оружия (Actor), который спаунится в РУКИ (стреляющий)
	// Например: BP_Rifle_Base
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TSubclassOf<class AODWeapon> WeaponActorClass;
};