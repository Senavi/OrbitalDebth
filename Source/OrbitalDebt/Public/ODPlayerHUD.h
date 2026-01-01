#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ODPlayerHUD.generated.h"

class UTextBlock;
// Предварительное объявление класса полоски
class UProgressBar;

UCLASS()
class ORBITALDEBT_API UODPlayerHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Функция для обновления здоровья (вызовем из Персонажа)
	void UpdateHealth(float CurrentHealth, float MaxHealth);
	
	// НОВОЕ: Функция обновления патронов
	void UpdateAmmo(int32 CurrentAmmo, int32 MaxAmmo);
	
	// Функция: Показать или Спрятать подсказку
	void SetInteractionPrompt(bool bIsVisible, FText Text = FText::GetEmpty());

protected:
	// --- ВИЗУАЛЬНЫЕ ЭЛЕМЕНТЫ ---
	
	// Полоска здоровья.
	// meta = (BindWidget) означает: "Найди в дизайнере элемент с именем HealthBar и подключи сюда".
	// Если имена не совпадут, игра упадет с ошибкой (что хорошо, сразу увидим баг).
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;
	
	// НОВОЕ: Текст патронов (свяжем с виджетом AmmoText)
	UPROPERTY(meta = (BindWidget))
	UTextBlock* AmmoText;
	
	// Текст подсказки ("Нажми F, чтобы подобрать...")
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* InteractionText;
};