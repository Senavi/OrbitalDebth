#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ODInventoryComponent.h"
#include "ODInventoryWindow.generated.h"

class UWrapBox;
class UPanelWidget; // Базовый класс для контейнеров (HorizontalBox, GridPanel и т.д.)
class UODInventorySlot;

UCLASS()
class ORBITALDEBT_API UODInventoryWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	void RefreshInventory(const TArray<FInventoryItem>& Items);

protected:
	// Сетка для лута (Рюкзак)
	UPROPERTY(meta = (BindWidget))
	UWrapBox* ItemGrid;

	// Контейнер для 8 слотов хотбара (Horizontal Box или Grid Panel)
	// В Дизайнере назовите этот контейнер "HotbarGrid"
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* HotbarGrid;
    
	// Отдельный слот для брони
	// В Дизайнере назовите слот "Slot_Armor"
	UPROPERTY(meta = (BindWidget))
	UODInventorySlot* Slot_Armor;
    
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UODInventorySlot> SlotWidgetClass;
};