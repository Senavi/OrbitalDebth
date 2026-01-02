#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ODItemData.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
    General     UMETA(DisplayName = "General"),
    Weapon      UMETA(DisplayName = "Weapon"),
    Armor       UMETA(DisplayName = "Armor"),
    Consumable  UMETA(DisplayName = "Consumable")
};

// НОВЫЕ ТИПЫ СЛОТОВ
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
    None    UMETA(DisplayName = "Rucksack / None"), // Рюкзак (индекс в массиве Items)
    Hotbar  UMETA(DisplayName = "Hotbar (1-8)"),    // Быстрые слоты (индекс 0-7 в HotbarItems)
    Armor   UMETA(DisplayName = "Armor Body")       // Броня (отдельный слот)
};

UCLASS()
class ORBITALDEBT_API UODItemData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Data")
    FText ItemName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Data", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Data")
    EItemType ItemType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory UI")
    UTexture2D* Icon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory UI")
    int32 Width = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory UI")
    int32 Height = 1;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    float Weight = 0.5f;

    // --- 3D МИР ---
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
    TSubclassOf<class AActor> ItemClass;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
    UStaticMesh* PickupMesh;
    
    // Куда это можно надеть?
    UPROPERTY(EditDefaultsOnly, Category = "Equipment")
    EEquipmentSlot EquipSlotType; 
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
    TSubclassOf<class AODWeapon> WeaponActorClass;
};