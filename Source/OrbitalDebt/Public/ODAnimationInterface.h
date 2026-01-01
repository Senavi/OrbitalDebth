#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ODAnimationInterface.generated.h"

UINTERFACE(MinimalAPI)
class UODAnimationInterface : public UInterface
{
	GENERATED_BODY()
};

class ORBITALDEBT_API IODAnimationInterface
{
	GENERATED_BODY()

public:
	// Мы используем "const", так как эти функции только читают данные
    
	// Целимся ли мы?
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation")
	bool GetIsAiming() const;

	// Бежим ли мы?
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation")
	bool GetIsSprinting() const;
    
	// (Опционально) Есть ли оружие в руках?
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation")
	bool HasWeapon() const;
	
	// Возвращает угол наклона прицеливания для Aim Offset
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation")
	float GetAimPitch() const;
};