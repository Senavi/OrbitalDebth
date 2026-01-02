#include "ODInventoryWindow.h"
#include "ODInventorySlot.h"
#include "Components/WrapBox.h"
#include "Components/PanelWidget.h"
#include "ODInventoryComponent.h"
#include "ODCharacter.h" 

void UODInventoryWindow::RefreshInventory(const TArray<FInventoryItem>& Items)
{
	UODInventoryComponent* InventoryComp = nullptr;
    
	if (GetOwningPlayerPawn())
	{
		if (AODCharacter* Char = Cast<AODCharacter>(GetOwningPlayerPawn()))
		{
			InventoryComp = Char->InventoryComponent;
		}
	}

	if (!InventoryComp) return;

	// 1. --- ОБНОВЛЕНИЕ ХОТБАРА (1-8) ---
	if (HotbarGrid)
	{
		for (int32 i = 0; i < HotbarGrid->GetChildrenCount(); i++)
		{
			// ИСПРАВЛЕНИЕ: Используем имя CurrentSlot вместо Slot
			if (UODInventorySlot* CurrentSlot = Cast<UODInventorySlot>(HotbarGrid->GetChildAt(i)))
			{
				FInventoryItem HotbarItem;
				// Получаем данные по индексу i (0-7)
				bool bHasItem = InventoryComp->GetHotbarItem(i, HotbarItem);
				
				if (bHasItem)
				{
					CurrentSlot->InitSlot(HotbarItem.ItemData, HotbarItem.Quantity, EEquipmentSlot::Hotbar, i);
				}
				else
				{
					CurrentSlot->InitSlot(nullptr, 0, EEquipmentSlot::Hotbar, i);
				}
			}
		}
	}

	// 2. --- ОБНОВЛЕНИЕ БРОНИ ---
	if (Slot_Armor)
	{
		FInventoryItem ArmorInfo = InventoryComp->GetArmorItem();
		if (ArmorInfo.IsValid())
		{
			Slot_Armor->InitSlot(ArmorInfo.ItemData, ArmorInfo.Quantity, EEquipmentSlot::Armor, 0);
		}
		else
		{
			Slot_Armor->InitSlot(nullptr, 0, EEquipmentSlot::Armor, 0);
		}
	}

	// 3. --- ОБНОВЛЕНИЕ РЮКЗАКА ---
	if (ItemGrid && SlotWidgetClass)
	{
		ItemGrid->ClearChildren(); 
        
		const TArray<FInventoryItem>& BackpackItems = InventoryComp->GetItems();
        
		for (int32 i = 0; i < BackpackItems.Num(); i++)
		{
			UODInventorySlot* NewSlot = CreateWidget<UODInventorySlot>(this, SlotWidgetClass);
			if (NewSlot)
			{
				NewSlot->InitSlot(BackpackItems[i].ItemData, BackpackItems[i].Quantity, EEquipmentSlot::None, i);
				ItemGrid->AddChildToWrapBox(NewSlot);
			}
		}
	}
}