#pragma once

#include "ODWeapon.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ODItemData.h" 
#include "ODInventoryComponent.generated.h"

// Добавь forward declaration вверху, если нет
class AODWeapon;

// Структура конкретного предмета
USTRUCT(BlueprintType)
struct FInventoryItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UODItemData* ItemData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 1;

    // НОВОЕ: Состояние магазина (-1 означает "По умолчанию/Полный")
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AmmoState = -1; 

    bool IsValid() const { return ItemData != nullptr; }
    
    // При очистке сбрасываем и патроны
    void Clear() { ItemData = nullptr; Quantity = 0; AmmoState = -1; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ORBITALDEBT_API UODInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public: 
    UODInventoryComponent();

protected:
    virtual void BeginPlay() override;

    // --- НАСТРОЙКИ ---
    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    int32 Capacity = 20;

    // --- СЛОТЫ ЭКИПИРОВКИ (НОВОЕ) ---
    // Это отдельные переменные, не входящие в общий массив Items
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
    FInventoryItem PrimaryWeapon;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
    FInventoryItem SecondaryWeapon;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
    FInventoryItem ArmorChest; // Бронежилет

    // --- РЮКЗАК (BACKPACK) ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TArray<FInventoryItem> Items;
    
    // Ссылка на реальное оружие в руках (которое сейчас заспаунено)
    UPROPERTY()
    AODWeapon* SpawnedPrimaryWeapon;
    
    // Допоміжна функція для отримання посилання на структуру по слоту
    FInventoryItem* GetItemSlot(EEquipmentSlot Slot, int32 Index);

public: 
    // --- ФУНКЦИИ ---
    
    // Обновляем TryAddItem, добавляем параметр Ammo (по умолчанию -1)
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool TryAddItem(UODItemData* ItemToAdd, int32 Quantity = 1, int32 Ammo = -1);
    
    // НОВОЕ: Выбросить предмет из конкретного слота экипировки
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void DropEquippedItem(EEquipmentSlot SlotType);
    

    // НОВОЕ: Внутренняя функция надевания предмета
    bool EquipItem(FInventoryItem* ItemInfo);

    // Геттеры для UI
    const TArray<FInventoryItem>& GetItems() const { return Items; }
    
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    FInventoryItem GetPrimaryWeapon() const { return PrimaryWeapon; }

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryUpdated OnInventoryUpdated;
    
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void DropItem(int32 ItemIndex);
    
    // Геттер, чтобы Персонаж мог получить доступ к оружию для стрельбы
    UFUNCTION(BlueprintCallable)
    AODWeapon* GetCurrentWeapon() const { return SpawnedPrimaryWeapon; }
    
    // Поточний активний слот зброї
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
    EEquipmentSlot ActiveWeaponSlot = EEquipmentSlot::Primary;

    // Головна функція для Drag & Drop переміщення
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void TransferItem(EEquipmentSlot SourceSlot, int32 SourceIndex, EEquipmentSlot TargetSlot, int32 TargetIndex);

    // Функція перемикання зброї
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SwitchWeapon(EEquipmentSlot NewSlot);
};