#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ODInteractInterface.generated.h"

// Это стандартная обертка Unreal (не трогаем)
UINTERFACE(MinimalAPI)
class UODInteractInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Класс интерфейса, который мы будем добавлять к предметам
 */
class ORBITALDEBT_API IODInteractInterface
{
	GENERATED_BODY()

public:
	// Добавляем BlueprintNativeEvent. 
	// Это говорит движку: "Создай функцию Execute_Interact, которую можно вызывать и из C++, и из Блюпринтов".
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(class AODCharacter* Character);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractText();
};