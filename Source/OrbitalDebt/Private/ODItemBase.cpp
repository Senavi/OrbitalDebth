// Fill out your copyright notice in the Description page of Project Settings.


#include "ODItemBase.h"
#include "ODItemData.h"
#include "ODCharacter.h"        // Чтобы видеть персонажа
#include "ODInventoryComponent.h"

// Sets default values
AODItemBase::AODItemBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Создаем меш
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = MeshComp;
	
	// Включаем физику, чтобы предмет падал на пол
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));

}

// Called when the game starts or when spawned
void AODItemBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AODItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Замени AODItemBase::Interact на AODItemBase::Interact_Implementation
void AODItemBase::Interact_Implementation(AODCharacter* Character)
{
	if (ItemData && Character)
	{
		// Получаем компонент инвентаря у персонажа
		if (UODInventoryComponent* Inventory = Character->InventoryComponent)
		{
			// Пытаемся добавить
			// Передаем AmmoState обратно в инвентарь
			bool bAdded = Inventory->TryAddItem(ItemData, ItemQuantity, ItemAmmoState);

			if (bAdded)
			{
				UE_LOG(LogTemp, Warning, TEXT("Picked up: %s"), *ItemData->ItemName.ToString());
				
				// Уничтожаем предмет в мире ТОЛЬКО если он поместился в карман
				Destroy();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Inventory is Full! Can't pickup."));
				// Тут можно вывести сообщение на экран: "No Space"
			}
		}
	}
}

FText AODItemBase::GetInteractText_Implementation()
{
	// Если паспорт есть - берем имя оттуда
	if (ItemData)
	{
		return FText::Format(NSLOCTEXT("Interaction", "Pickup", "Pick up {0}"), ItemData->ItemName);
	}
	
	// Если забыли назначить - пишем ошибку
	return FText::FromString("INVALID ITEM DATA");
}

void AODItemBase::InitDrop(UODItemData* InitData, int32 Quantity, int32 Ammo)
{
	if (InitData)
	{
		ItemData = InitData;
		ItemQuantity = Quantity;
		ItemAmmoState = Ammo; // <--- Запоминаем патроны
		
		// 1. Меняем внешний вид (меш) на тот, что указан в DataAsset
		if (InitData->PickupMesh)
		{
			MeshComp->SetStaticMesh(InitData->PickupMesh);
		}

		// --- ФИКС ЛЕВИТАЦИИ ---
		// 2. Принудительно включаем физику и коллизию
		// QueryAndPhysics = предмет блокирует движение и реагирует на физику
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetCollisionProfileName(TEXT("PhysicsActor")); // Или "BlockAll"
		MeshComp->SetSimulatePhysics(true);
       
		// 3. Добавляем толчок вперед, чтобы оружие красиво вылетало от игрока
		if (GetOwner())
		{
			FVector Forward = GetOwner()->GetActorForwardVector();
			// Импульс толкает предмет вперед
			MeshComp->AddImpulse(Forward * 300.0f, NAME_None, true); 
		}
	}
}