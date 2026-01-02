#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ODItemData.h" // Чтобы знать структуру данных
#include "ODInventorySlot.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class ORBITALDEBT_API UODInventorySlot : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	// Тип слота. EditAnywhere = можно менять в дизайнере.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot Data")
	EEquipmentSlot AssignedSlotType = EEquipmentSlot::None;

	// Индекс. EditAnywhere = можно менять в дизайнере.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot Data")
	int32 BackpackIndex = -1;

	// --- ДОБАВЬ ВОТ ЭТУ СТРОКУ НИЖЕ: ---
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitSlot(UODItemData* InData, int32 InQty, EEquipmentSlot InType, int32 InIndex);
	
	// КЭШ: Запоминаем данные для Drag & Drop
	UPROPERTY(BlueprintReadOnly, Category = "Slot Data")
	UODItemData* StoredItemData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Slot Data")
	int32 StoredItemQuantity = 0;

protected:
	// Картинка предмета
	UPROPERTY(meta = (BindWidget))
	UImage* IconImage;

	// Текст количества (x5, x30)
	UPROPERTY(meta = (BindWidget))
	UTextBlock* QuantityText;
};