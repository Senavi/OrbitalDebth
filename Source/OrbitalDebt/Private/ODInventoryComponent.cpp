#include "ODInventoryComponent.h"
#include "ODItemBase.h"
#include "ODWeapon.h" 
#include "GameFramework/Character.h" 

UODInventoryComponent::UODInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UODInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	// Инициализируем 8 пустых слотов хотбара
	HotbarItems.SetNum(8);
}

FInventoryItem* UODInventoryComponent::GetSlotReference(EEquipmentSlot SlotType, int32 Index)
{
	if (SlotType == EEquipmentSlot::None)
	{
		if (Items.IsValidIndex(Index)) return &Items[Index];
	}
	else if (SlotType == EEquipmentSlot::Hotbar)
	{
		if (HotbarItems.IsValidIndex(Index)) return &HotbarItems[Index];
	}
	else if (SlotType == EEquipmentSlot::Armor)
	{
		return &ArmorItem;
	}
	return nullptr;
}

bool UODInventoryComponent::TryAddItem(UODItemData* ItemToAdd, int32 Quantity, int32 Ammo)
{
	if (!ItemToAdd) return false;

	// 1. Пробуем положить в Броню
	if (ItemToAdd->EquipSlotType == EEquipmentSlot::Armor && !ArmorItem.IsValid())
	{
		ArmorItem.ItemData = ItemToAdd;
		ArmorItem.Quantity = 1;
		ArmorItem.AmmoState = -1;
		OnInventoryUpdated.Broadcast();
		return true;
	}

	// 2. Пробуем положить в первый свободный слот Хотбара
	if (ItemToAdd->EquipSlotType == EEquipmentSlot::Hotbar)
	{
		for (int32 i = 0; i < HotbarItems.Num(); i++)
		{
			if (!HotbarItems[i].IsValid())
			{
				HotbarItems[i].ItemData = ItemToAdd;
				HotbarItems[i].Quantity = Quantity;
				HotbarItems[i].AmmoState = Ammo;
				
				// Если мы положили оружие и руки были пусты - берем его
				if (!SpawnedWeaponActor && ItemToAdd->ItemType == EItemType::Weapon)
				{
					SelectHotbarSlot(i);
				}
				
				OnInventoryUpdated.Broadcast();
				return true;
			}
		}
	}

	// 3. Кладем в Рюкзак (Стакинг)
	for (FInventoryItem& Item : Items)
	{
		if (Item.ItemData == ItemToAdd)
		{
			Item.Quantity += Quantity;
			OnInventoryUpdated.Broadcast();
			return true;
		}
	}

	// 4. Новый слот в рюкзаке
	if (Items.Num() < Capacity)
	{
		FInventoryItem NewItem;
		NewItem.ItemData = ItemToAdd;
		NewItem.Quantity = Quantity;
		NewItem.AmmoState = Ammo;
		Items.Add(NewItem);
		OnInventoryUpdated.Broadcast();
		return true;
	}

	return false; 
}

void UODInventoryComponent::TransferItem(EEquipmentSlot SourceSlot, int32 SourceIndex, EEquipmentSlot TargetSlot, int32 TargetIndex)
{
	FInventoryItem* Source = GetSlotReference(SourceSlot, SourceIndex);
	FInventoryItem* Target = GetSlotReference(TargetSlot, TargetIndex);

	if (!Source || !Source->IsValid()) return;
    
	// Проверка типов
	if (TargetSlot == EEquipmentSlot::Armor)
	{
		if (Source->ItemData->EquipSlotType != EEquipmentSlot::Armor) return;
	}
	else if (TargetSlot == EEquipmentSlot::Hotbar)
	{
		// В хотбар нельзя броню
		if (Source->ItemData->EquipSlotType == EEquipmentSlot::Armor) return; 
	}

	// Создание слота в рюкзаке, если кинули в пустоту
	if (TargetSlot == EEquipmentSlot::None && !Target)
	{
		Items.Add(*Source);
		
		if (SourceSlot == EEquipmentSlot::None) Items.RemoveAt(SourceIndex);
		else Source->Clear();

		// Если убрали активное оружие
		if (SourceSlot == EEquipmentSlot::Hotbar && SourceIndex == ActiveHotbarIndex)
		{
			if (SpawnedWeaponActor) 
			{
				SpawnedWeaponActor->Destroy();
				SpawnedWeaponActor = nullptr;
			}
		}
		
		OnInventoryUpdated.Broadcast();
		return;
	}

	if (!Target) return;

	// --- SWAP ---
	FInventoryItem Temp = *Target;
	*Target = *Source;
	*Source = Temp;

	// Удаление пустых из рюкзака
	if (SourceSlot == EEquipmentSlot::None && !Source->IsValid())
	{
		Items.RemoveAt(SourceIndex);
	}
	if (TargetSlot == EEquipmentSlot::None && !Target->IsValid())
	{
		Items.RemoveAt(TargetIndex);
	}

	// Обновление оружия в руках
	if ((SourceSlot == EEquipmentSlot::Hotbar && SourceIndex == ActiveHotbarIndex) ||
		(TargetSlot == EEquipmentSlot::Hotbar && TargetIndex == ActiveHotbarIndex))
	{
		SelectHotbarSlot(ActiveHotbarIndex);
	}

	OnInventoryUpdated.Broadcast();
}

void UODInventoryComponent::SelectHotbarSlot(int32 SlotIndex)
{
	if (!HotbarItems.IsValidIndex(SlotIndex)) return;

	ActiveHotbarIndex = SlotIndex;
	FInventoryItem& SelectedItem = HotbarItems[ActiveHotbarIndex];

	// 1. Удаляем старое
	if (SpawnedWeaponActor)
	{
		SpawnedWeaponActor->Destroy();
		SpawnedWeaponActor = nullptr;
	}

	// 2. Спауним новое (если это оружие)
	if (SelectedItem.IsValid() && SelectedItem.ItemData->ItemType == EItemType::Weapon)
	{
		EquipWeaponActor(SelectedItem);
	}
}

void UODInventoryComponent::EquipWeaponActor(const FInventoryItem& ItemInfo)
{
	if (!ItemInfo.ItemData || !ItemInfo.ItemData->WeaponActorClass) return;

	if (AActor* Owner = GetOwner())
	{
		 FActorSpawnParameters Params;
		 Params.Owner = Owner;
		 Params.Instigator = Cast<APawn>(Owner);
		 
		 SpawnedWeaponActor = GetWorld()->SpawnActor<AODWeapon>(ItemInfo.ItemData->WeaponActorClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
		 
		 if (SpawnedWeaponActor)
		 {
			 if (ACharacter* Char = Cast<ACharacter>(Owner))
			 {
				SpawnedWeaponActor->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("WeaponSocket"));
				
				if (ItemInfo.AmmoState >= 0)
				{
					SpawnedWeaponActor->CurrentAmmo = ItemInfo.AmmoState;
				}
				
				if (AODCharacter* ODChar = Cast<AODCharacter>(Char))
				{
					ODChar->UpdateAmmoHUD();
				}
			 }
		 }
	}
}

void UODInventoryComponent::DropActiveWeapon()
{
	if (ActiveHotbarIndex >= 0 && HotbarItems.IsValidIndex(ActiveHotbarIndex))
	{
		FInventoryItem& ItemToDrop = HotbarItems[ActiveHotbarIndex];
		
		if (!ItemToDrop.IsValid()) return; // Пустой слот

		if (SpawnedWeaponActor)
		{
			ItemToDrop.AmmoState = SpawnedWeaponActor->CurrentAmmo;
			SpawnedWeaponActor->Destroy();
			SpawnedWeaponActor = nullptr;
		}

		if (GetOwner())
		{
			FVector SpawnLoc = GetOwner()->GetActorLocation() + (GetOwner()->GetActorForwardVector() * 100.0f);
			AActor* Pickup = GetWorld()->SpawnActor<AActor>(ItemToDrop.ItemData->ItemClass, SpawnLoc, FRotator::ZeroRotator);
			if (AODItemBase* BasePickup = Cast<AODItemBase>(Pickup))
			{
				BasePickup->InitDrop(ItemToDrop.ItemData, ItemToDrop.Quantity, ItemToDrop.AmmoState);
			}
		}

		ItemToDrop.Clear();
		OnInventoryUpdated.Broadcast();
	}
}

void UODInventoryComponent::DropItem(int32 ItemIndex)
{
	if (Items.IsValidIndex(ItemIndex))
	{
		FInventoryItem ItemToDrop = Items[ItemIndex];
		if (GetOwner())
		{
			FVector SpawnLoc = GetOwner()->GetActorLocation() + (GetOwner()->GetActorForwardVector() * 100.0f);
			AActor* Pickup = GetWorld()->SpawnActor<AActor>(ItemToDrop.ItemData->ItemClass, SpawnLoc, FRotator::ZeroRotator);
			if (AODItemBase* BasePickup = Cast<AODItemBase>(Pickup))
			{
				BasePickup->InitDrop(ItemToDrop.ItemData, ItemToDrop.Quantity, ItemToDrop.AmmoState);
			}
		}

		Items.RemoveAt(ItemIndex);
		OnInventoryUpdated.Broadcast();
	}
}

bool UODInventoryComponent::GetHotbarItem(int32 Index, FInventoryItem& OutItem) const
{
	if (HotbarItems.IsValidIndex(Index))
	{
		OutItem = HotbarItems[Index];
		return true;
	}
	return false;
}