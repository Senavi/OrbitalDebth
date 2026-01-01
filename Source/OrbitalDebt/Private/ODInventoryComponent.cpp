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
}

bool UODInventoryComponent::TryAddItem(UODItemData* ItemToAdd, int32 Quantity, int32 Ammo)
{
    if (!ItemToAdd) return false;

    // --- СОЗДАЕМ ВРЕМЕННУЮ СТРУКТУРУ ---
    // Мы "упаковываем" данные, чтобы передать их в функцию экипировки
    FInventoryItem TempItem;
    TempItem.ItemData = ItemToAdd;
    TempItem.Quantity = Quantity;
    TempItem.AmmoState = Ammo;

    // --- ЭТАП 1: АВТО-ЭКИПИРОВКА ---
    
    // 1. Основное оружие
    if (ItemToAdd->EquipSlotType == EEquipmentSlot::Primary)
    {
        if (!PrimaryWeapon.IsValid()) 
        {
            // Передаем адрес временной структуры (&TempItem)
            EquipItem(&TempItem); 
            return true;
        }
    }
    // 2. Вторичное оружие
    else if (ItemToAdd->EquipSlotType == EEquipmentSlot::Secondary)
    {
        if (!SecondaryWeapon.IsValid())
        {
            EquipItem(&TempItem); // Передаем структуру
            return true;
        }
    }
    // 3. Броня
    else if (ItemToAdd->EquipSlotType == EEquipmentSlot::Body)
    {
        if (!ArmorChest.IsValid())
        {
            EquipItem(&TempItem); // Передаем структуру
            return true;
        }
    }

    // --- ЭТАП 2: РЮКЗАК ---
    
    // 1. Стакинг
    for (FInventoryItem& Item : Items)
    {
        if (Item.ItemData == ItemToAdd)
        {
            Item.Quantity += Quantity; // Прибавляем кол-во
            OnInventoryUpdated.Broadcast();
            return true;
        }
    }

    // 2. Новый слот
    if (Items.Num() >= Capacity)
    {
        return false;
    }

    // Добавляем новую запись с учетом патронов
    FInventoryItem NewItem;
    NewItem.ItemData = ItemToAdd;
    NewItem.Quantity = Quantity;
    NewItem.AmmoState = Ammo; // Записываем патроны
    
    Items.Add(NewItem);

    OnInventoryUpdated.Broadcast();
    return true;
}
bool UODInventoryComponent::EquipItem(FInventoryItem* ItemInfo)
{
    // 1. Проверка валидности
    if (!ItemInfo || !ItemInfo->IsValid()) return false;
    
    // Достаем DataAsset из структуры для удобства
    UODItemData* Data = ItemInfo->ItemData;

    // --- PRIMARY WEAPON ---
    if (Data->EquipSlotType == EEquipmentSlot::Primary)
    {
        // Сохраняем структуру целиком (вместе с патронами)
        PrimaryWeapon = *ItemInfo; 
        
        // Спауним
        if (Data->WeaponActorClass)
        {
            if (SpawnedPrimaryWeapon) SpawnedPrimaryWeapon->Destroy();

            if (AActor* Owner = GetOwner())
            {
                 FActorSpawnParameters Params;
                 Params.Owner = Owner;
                 Params.Instigator = Cast<APawn>(Owner);
                 
                 SpawnedPrimaryWeapon = GetWorld()->SpawnActor<AODWeapon>(Data->WeaponActorClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
                 
                 if (SpawnedPrimaryWeapon)
                 {
                     if (ACharacter* Char = Cast<ACharacter>(Owner))
                     {
                        SpawnedPrimaryWeapon->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("WeaponSocket"));
                        
                        // --- ПРИМЕНЯЕМ ПАТРОНЫ ---
                        // Если в структуре записано кол-во патронов (>=0), ставим его.
                        if (ItemInfo->AmmoState >= 0)
                        {
                            SpawnedPrimaryWeapon->CurrentAmmo = ItemInfo->AmmoState;
                        }
                        
                        // Обновляем HUD
                        if (AODCharacter* ODChar = Cast<AODCharacter>(Char))
                        {
                            ODChar->UpdateAmmoHUD();
                        }
                     }
                 }
            }
        }
    }
    // Сюда можно добавить блоки else if для Secondary и Armor, если нужно

    OnInventoryUpdated.Broadcast();
    return true;
}

void UODInventoryComponent::DropItem(int32 ItemIndex)
{
    if (!Items.IsValidIndex(ItemIndex)) return;

    FInventoryItem& ItemToDrop = Items[ItemIndex];
    
    // Запоминаем, сколько всего предметов в стаке (например, 5)
    int32 TotalQuantity = ItemToDrop.Quantity; 

    if (AActor* Owner = GetOwner())
    {
        if (ItemToDrop.ItemData && ItemToDrop.ItemData->ItemClass)
        {
            // --- ЦИКЛ РАССЫПАНИЯ ---
            // Выполняем спаун столько раз, сколько предметов в стаке
            for (int32 i = 0; i < TotalQuantity; i++)
            {
                // Делаем небольшой разброс координат, чтобы предметы не застряли друг в друге
                float RandomX = FMath::RandRange(-30.0f, 30.0f);
                float RandomY = FMath::RandRange(-30.0f, 30.0f);
               
                // Спауним перед игроком + случайное смещение
                FVector SpawnLoc = Owner->GetActorLocation() 
                                 + (Owner->GetActorForwardVector() * 100.0f) 
                                 + FVector(RandomX, RandomY, 50);

                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                // Спауним со случайным поворотом
                AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
                    ItemToDrop.ItemData->ItemClass, 
                    SpawnLoc, 
                    FRotator(FMath::RandRange(0.0f, 360.0f), FMath::RandRange(0.0f, 360.0f), 0.0f), 
                    Params
                );
              
                if (AODItemBase* Pickup = Cast<AODItemBase>(SpawnedActor))
                {
                    // ВАЖНО: Каждому предмету ставим количество 1 !
                    Pickup->InitDrop(ItemToDrop.ItemData, 1);
                }
            }
        }
    }
    
    // Удаляем стак из инвентаря
    Items.RemoveAt(ItemIndex);
    OnInventoryUpdated.Broadcast();
}

void UODInventoryComponent::DropEquippedItem(EEquipmentSlot SlotType)
{
    FInventoryItem* ItemToDrop = nullptr;
    int32 CurrentWeaponAmmo = -1; // По умолчанию
    
    if (SlotType == EEquipmentSlot::Primary)
    {
        ItemToDrop = &PrimaryWeapon;
        // Если оружие существует, запоминаем сколько в нем патронов перед уничтожением
        if (SpawnedPrimaryWeapon)
        {
            CurrentWeaponAmmo = SpawnedPrimaryWeapon->CurrentAmmo;
        }
    }
    
    // Если слота нет или он пуст - выходим
    if (!ItemToDrop || !ItemToDrop->IsValid()) return;

    // 2. Спауним коробку в мире
    if (AActor* Owner = GetOwner())
    {
        FVector SpawnLoc = Owner->GetActorLocation() + (Owner->GetActorForwardVector() * 100.0f) + FVector(0,0,50);
        
        if (ItemToDrop->ItemData && ItemToDrop->ItemData->ItemClass)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
             
            AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ItemToDrop->ItemData->ItemClass, SpawnLoc, FRotator::ZeroRotator, Params);
             
            if (AODItemBase* Pickup = Cast<AODItemBase>(SpawnedActor))
            {
                Pickup->InitDrop(ItemToDrop->ItemData, ItemToDrop->Quantity, CurrentWeaponAmmo);
            }
        }
    }

    // 3. УНИЧТОЖАЕМ ОРУЖИЕ В РУКАХ (Если это Primary)
    if (SlotType == EEquipmentSlot::Primary && SpawnedPrimaryWeapon)
    {
        SpawnedPrimaryWeapon->Destroy();
        SpawnedPrimaryWeapon = nullptr;
    }

    // 4. Очищаем данные слота
    ItemToDrop->Clear();

    // 5. Обновляем UI
    OnInventoryUpdated.Broadcast();
    
    UE_LOG(LogTemp, Warning, TEXT("Dropped Equipped Item!"));
}