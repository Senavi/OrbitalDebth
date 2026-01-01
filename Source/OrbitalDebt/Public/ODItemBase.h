// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "ODInteractInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ODItemBase.generated.h"

class UODItemData;

UCLASS()
class ORBITALDEBT_API AODItemBase : public AActor, public IODInteractInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AODItemBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Компонент меша (чтобы менять вид)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	UStaticMeshComponent* MeshComp;
	
	// Кол-во предметов
	UPROPERTY(VisibleAnywhere, Category = "Item")
	int32 ItemQuantity = 1;

	// НОВОЕ: Патроны внутри (-1 = полная)
	UPROPERTY(VisibleAnywhere, Category = "Item")
	int32 ItemAmmoState = -1;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// Обрати внимание на суффикс _Implementation
	virtual void Interact_Implementation(class AODCharacter* Character) override;
	virtual FText GetInteractText_Implementation() override;
	
	// Ссылка на Паспорт предмета
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UODItemData> ItemData;
	
	// Обновляем сигнатуру
	void InitDrop(UODItemData* InitData, int32 Quantity, int32 Ammo = -1);

};
