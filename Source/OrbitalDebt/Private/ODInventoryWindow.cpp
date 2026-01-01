#include "ODInventoryWindow.h"
#include "ODInventorySlot.h"
#include "Components/WrapBox.h"
#include "ODInventoryComponent.h"
#include "ODCharacter.h" // Добавляем, чтобы скастить PlayerPawn

void UODInventoryWindow::RefreshInventory(const TArray<FInventoryItem>& Items)
{
	// Сразу ищем компонент инвентаря у игрока
	UODInventoryComponent* InventoryComp = nullptr;
    
	if (GetOwningPlayerPawn())
	{
		if (AODCharacter* Char = Cast<AODCharacter>(GetOwningPlayerPawn()))
		{
			InventoryComp = Char->InventoryComponent;
		}
	}

	if (!InventoryComp) return;

	// --- ЧАСТЬ 1: ОБНОВЛЯЕМ ЭКИПИРОВКУ (Верхний слот) ---
	if (Slot_PrimaryWeapon)
	{
		FInventoryItem PrimaryItem = InventoryComp->GetPrimaryWeapon();
       
		// Инициализируем слот как PRIMARY (Индекс -1, так как это не массив)
		Slot_PrimaryWeapon->InitSlot(PrimaryItem.ItemData, PrimaryItem.Quantity, EEquipmentSlot::Primary, -1);
	}

	// --- ЧАСТЬ 2: ОБНОВЛЯЕМ РЮКЗАК (Сетка внизу) ---
	if (ItemGrid && SlotWidgetClass)
	{
		ItemGrid->ClearChildren(); // Очищаем старые иконки
        
		// Берем предметы из рюкзака
		const TArray<FInventoryItem>& BackpackItems = InventoryComp->GetItems();
        
		for (int32 i = 0; i < BackpackItems.Num(); i++)
		{
			// Создаем виджет
			UODInventorySlot* NewSlot = CreateWidget<UODInventorySlot>(this, SlotWidgetClass);
			if (NewSlot)
			{
				// Инициализируем как РЮКЗАК (Тип None, Индекс = i)
				NewSlot->InitSlot(BackpackItems[i].ItemData, BackpackItems[i].Quantity, EEquipmentSlot::None, i);
             
				// Добавляем в сетку
				ItemGrid->AddChildToWrapBox(NewSlot);
			}
		}
	}
}