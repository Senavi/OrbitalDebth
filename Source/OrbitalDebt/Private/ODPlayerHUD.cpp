#include "ODPlayerHUD.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h" // Важный инклюд для работы с баром

void UODPlayerHUD::UpdateHealth(float CurrentHealth, float MaxHealth)
{
	if (HealthBar)
	{
		// Вычисляем процент (от 0.0 до 1.0)
		float Percent = CurrentHealth / MaxHealth;
		
		// Ставим процент
		HealthBar->SetPercent(Percent);
		
		// Можно менять цвет: Зеленый > 50%, Красный < 20% (опционально)
		if(Percent < 0.2f)
		{
			HealthBar->SetFillColorAndOpacity(FLinearColor::Red);
		}
		else
		{
			HealthBar->SetFillColorAndOpacity(FLinearColor::Green); // Или любой другой цвет
		}
	}
}

void UODPlayerHUD::UpdateAmmo(int32 CurrentAmmo, int32 MaxAmmo)
{
	if (AmmoText)
	{
		// Форматируем строку: "30 / 30"
		FString AmmoString = FString::Printf(TEXT("%d / %d"), CurrentAmmo, MaxAmmo);
		
		// Устанавливаем текст
		AmmoText->SetText(FText::FromString(AmmoString));
	}
}

void UODPlayerHUD::SetInteractionPrompt(bool bIsVisible, FText Text)
{
	if (InteractionText)
	{
		if (bIsVisible)
		{
			InteractionText->SetVisibility(ESlateVisibility::Visible);
			InteractionText->SetText(Text);
		}
		else
		{
			// Если не видим предмет - прячем текст (Hidden или Collapsed)
			InteractionText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}