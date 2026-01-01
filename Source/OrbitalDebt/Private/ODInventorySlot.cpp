// ODInventorySlot.cpp

#include "ODInventorySlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ODItemData.h" // Убедись, что этот инклюд есть

void UODInventorySlot::InitSlot(UODItemData* InData, int32 InQty, EEquipmentSlot InType, int32 InIndex)
{
	AssignedSlotType = InType;
	BackpackIndex = InIndex;
	
	// 1. ЗАПОМИНАЕМ (Кэшируем) входящие данные
	StoredItemData = InData;
	StoredItemQuantity = InQty;

	// ... дальше твой старый код ...
	AssignedSlotType = InType;
	BackpackIndex = InIndex;
    
	// ПРОВЕРКА: Есть ли данные о предмете?
	if (InData)
	{
		// --- ВЕТКА: ПРЕДМЕТ ЕСТЬ ---

		// 1. Ставим иконку
		if (IconImage)
		{
			if (InData->Icon)
			{
				IconImage->SetBrushFromTexture(InData->Icon);
				IconImage->SetVisibility(ESlateVisibility::Visible);
			}
		}

		// 2. Ставим количество
		if (QuantityText)
		{
			if (InQty > 1)
			{
				QuantityText->SetText(FText::AsNumber(InQty));
				QuantityText->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				QuantityText->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
	else 
	{
		// --- ВЕТКА: ПРЕДМЕТА НЕТ (ПУСТОЙ СЛОТ) ---
		// ВАЖНО: Мы должны принудительно скрыть иконку и текст!
       
		if (IconImage) 
		{
			// Ставим прозрачность или Hidden, чтобы картинка пропала
			IconImage->SetVisibility(ESlateVisibility::Hidden);
		}
       
		if (QuantityText) 
		{
			QuantityText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}