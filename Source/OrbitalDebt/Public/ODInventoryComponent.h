#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ODItemData.h" 
#include "ODInventoryComponent.generated.h"

class AODWeapon;

USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UODItemData* ItemData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AmmoState = -1; 

	bool IsValid() const { return ItemData != nullptr; }
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

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	int32 Capacity = 20;

	// --- НОВАЯ СИСТЕМА ЭКИПИРОВКИ ---
    
	// Хотбар (8 слотов: индексы 0-7)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<FInventoryItem> HotbarItems;

	// Броня
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FInventoryItem ArmorItem;

	// Индекс активного слота (-1 если ничего не выбрано)
	int32 ActiveHotbarIndex = -1; 

	// Рюкзак
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryItem> Items;
    
	// Ссылка на заспауненное оружие в руках
	UPROPERTY()
	AODWeapon* SpawnedWeaponActor;

public: 
	// --- ФУНКЦИИ ---
    
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryAddItem(UODItemData* ItemToAdd, int32 Quantity = 1, int32 Ammo = -1);

	// Функция для Drag & Drop
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void TransferItem(EEquipmentSlot SourceSlot, int32 SourceIndex, EEquipmentSlot TargetSlot, int32 TargetIndex);

	// Выбор слота (1-8)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SelectHotbarSlot(int32 SlotIndex);

	// Выбросить активное оружие
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropActiveWeapon();
    
	// Выбросить предмет из рюкзака
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropItem(int32 ItemIndex);

	// Вспомогательная: Надеть предмет
	void EquipWeaponActor(const FInventoryItem& ItemInfo);

	// Геттеры для UI и Логики
	const TArray<FInventoryItem>& GetItems() const { return Items; }
    
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetHotbarItem(int32 Index, FInventoryItem& OutItem) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryItem GetArmorItem() const { return ArmorItem; }
	
	// Для ботов (получить весь массив хотбара для дропа)
	const TArray<FInventoryItem>& GetHotbarItems() const { return HotbarItems; }

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;
    
	UFUNCTION(BlueprintCallable)
	AODWeapon* GetCurrentWeapon() const { return SpawnedWeaponActor; }
    
private:
	// Получить указатель на слот по типу и индексу
	FInventoryItem* GetSlotReference(EEquipmentSlot SlotType, int32 Index);
};