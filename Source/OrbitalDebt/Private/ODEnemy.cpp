#include "ODEnemy.h"
#include "ODInventoryComponent.h"
#include "Components/CapsuleComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ODAIController.h" 
#include "ODWeapon.h"
#include "ODItemBase.h" 
#include "ODItemData.h" 

AODEnemy::AODEnemy()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AODAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	InventoryComponent = CreateDefaultSubobject<UODInventoryComponent>(TEXT("EnemyInventory"));
}

void AODEnemy::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth; 
	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
	
	if (InventoryComponent)
	{
		for (UODItemData* Item : DefaultLoadout)
		{
			InventoryComponent->TryAddItem(Item);
		}
	}
}

float AODEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
    
	if (CurrentHealth > 0.0f)
	{
		if (AODAIController* AICon = Cast<AODAIController>(GetController()))
		{
			if (EventInstigator && EventInstigator->GetPawn())
			{
				FVector ShotOrigin = EventInstigator->GetPawn()->GetActorLocation();
				AICon->GetBlackboardComponent()->SetValueAsVector(TEXT("SuspiciousLocation"), ShotOrigin);
				if (UBehaviorTreeComponent* BTC = Cast<UBehaviorTreeComponent>(AICon->GetBrainComponent()))
				{
					BTC->RestartTree();
				}
			}
		}
	}
	else Die();

	return ActualDamage;
}

void AODEnemy::Attack()
{
	if (!InventoryComponent) return;
	AODWeapon* Weapon = InventoryComponent->GetCurrentWeapon(); 
	
	bIsAiming = true;

	if (!Weapon) return; 

	if (Weapon->CurrentAmmo <= 0)
	{
		Reload();
		return;
	}

	Weapon->StartFire(); 
	float BurstTime = FMath::RandRange(0.2f, 0.6f);
	GetWorldTimerManager().SetTimer(TimerHandle_BurstStop, this, &AODEnemy::StopFire, BurstTime, false);
}

void AODEnemy::StopFire()
{
	if (InventoryComponent && InventoryComponent->GetCurrentWeapon())
	{
		InventoryComponent->GetCurrentWeapon()->StopFire();
	}
	bIsAiming = false;
}

void AODEnemy::Reload()
{
	if (InventoryComponent && InventoryComponent->GetCurrentWeapon())
	{
		AODWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
		Weapon->Reload();

		if (Weapon->ReloadMontage)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance) AnimInstance->Montage_Play(Weapon->ReloadMontage);
		}
	}
}

void AODEnemy::PlayFireAnimation()
{
	if (FireMontage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance) AnimInstance->Montage_Play(FireMontage);
	}
}

void AODEnemy::Die()
{
    if (GetLifeSpan() > 0.0f) return; 

    if (AController* AICon = GetController()) AICon->UnPossess(); 
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->SetComponentTickEnabled(false);
    if (GetMesh()->GetAnimInstance()) GetMesh()->GetAnimInstance()->StopAllMontages(0.0f);

    GetMesh()->SetSimulatePhysics(true);
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (InventoryComponent)
	{
		// 1. Дропаем все из Хотбара (включая оружие в руках)
		const TArray<FInventoryItem>& Hotbar = InventoryComponent->GetHotbarItems();
		for (const FInventoryItem& HItem : Hotbar)
		{
			if (HItem.IsValid() && HItem.ItemData->ItemClass)
			{
				FVector SpawnLoc = GetActorLocation() + FVector(FMath::RandRange(-30,30), FMath::RandRange(-30,30), 40);
				AActor* SpawnedLoot = GetWorld()->SpawnActor<AActor>(HItem.ItemData->ItemClass, SpawnLoc, FRotator::ZeroRotator);
				if (AODItemBase* Pickup = Cast<AODItemBase>(SpawnedLoot))
				{
					Pickup->InitDrop(HItem.ItemData, HItem.Quantity, HItem.AmmoState);
				}
			}
		}
		// 2. Уничтожаем визуал оружия в руках
		if (InventoryComponent->GetCurrentWeapon()) InventoryComponent->GetCurrentWeapon()->Destroy();


		// 3. Рюкзак
		TArray<FInventoryItem> EnemyItems = InventoryComponent->GetItems();
		for (const FInventoryItem& InvItem : EnemyItems)
		{
			if (InvItem.IsValid() && InvItem.ItemData->ItemClass)
			{
				int32 QtyToSpawn = InvItem.Quantity;
				for (int32 i = 0; i < QtyToSpawn; i++)
				{
					float RandomX = FMath::RandRange(-60.0f, 60.0f);
					float RandomY = FMath::RandRange(-60.0f, 60.0f);
					FVector SpawnLoc = GetActorLocation() + FVector(RandomX, RandomY, 40);
					AActor* SpawnedLoot = GetWorld()->SpawnActor<AActor>(InvItem.ItemData->ItemClass, SpawnLoc, FRotator(0,FMath::RandRange(0,360),0));
					if (AODItemBase* Pickup = Cast<AODItemBase>(SpawnedLoot))
					{
						Pickup->InitDrop(InvItem.ItemData, 1);
					}
				}
			}
		}
	}
	
    // 4. Лут таблица
    if (!LootTable.IsEmpty())
    {
       for (UODItemData* LootItem : LootTable)
       {
          if (LootItem && LootItem->ItemClass)
          {
             FVector SpawnLoc = GetActorLocation() + FVector(FMath::RandRange(-50,50), FMath::RandRange(-50,50), 40);
             AActor* SpawnedLoot = GetWorld()->SpawnActor<AActor>(LootItem->ItemClass, SpawnLoc, FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f));
             if (AODItemBase* Pickup = Cast<AODItemBase>(SpawnedLoot))
             {
                int32 RandomQty = FMath::RandRange(1, 3);
                Pickup->InitDrop(LootItem, RandomQty);
             }
          }
       }
    }
}

// Interfaces
bool AODEnemy::GetIsAiming_Implementation() const { return bIsAiming; }
bool AODEnemy::GetIsSprinting_Implementation() const { return bIsSprinting; }
bool AODEnemy::HasWeapon_Implementation() const { return (InventoryComponent && InventoryComponent->GetCurrentWeapon()); }

void AODEnemy::SetSprinting(bool bNewSprint)
{
	bIsSprinting = bNewSprint;
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? ChaseSpeed : PatrolSpeed;
}

float AODEnemy::GetAimPitch_Implementation() const
{
	FRotator Delta = GetBaseAimRotation() - GetActorRotation();
	Delta.Normalize();
	return Delta.Pitch;
}