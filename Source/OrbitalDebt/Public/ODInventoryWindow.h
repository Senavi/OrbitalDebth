#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ODInventoryComponent.h" // Чтобы видеть структуру FInventoryItem
#include "ODInventoryWindow.generated.h"

class UWrapBox; // Контейнер, который сам переносит элементы на новую строку

UCLASS()
class ORBITALDEBT_API UODInventoryWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	// Главная функция: очищает сетку и рисует всё заново
	void RefreshInventory(const TArray<FInventoryItem>& Items);

protected:
	// Сетка (контейнер)
	UPROPERTY(meta = (BindWidget))
	UWrapBox* ItemGrid;

	// Какой виджет создавать для каждого предмета (WBP_InventorySlot)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<class UODInventorySlot> SlotWidgetClass;
	
	// НОВОЕ: Отдельный виджет для слота оружия (добавим его в дизайнере)
	UPROPERTY(meta = (BindWidget))
	class UODInventorySlot* Slot_PrimaryWeapon;
};