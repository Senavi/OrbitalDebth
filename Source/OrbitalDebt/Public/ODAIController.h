#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h" // Важный тип данных
#include "ODAIController.generated.h"

// Предварительные объявления
class UBehaviorTree;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS()
class ORBITALDEBT_API AODAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AODAIController(); // Конструктор (тут настроим зрение)

	// Ссылка на Дерево
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	// --- ЗРЕНИЕ ---

	// Компонент восприятия
	UPROPERTY(VisibleAnywhere, Category = "AI")
	UAIPerceptionComponent* AIPerception;

	// Настройка зрения (дальность, угол)
	UPROPERTY(VisibleAnywhere, Category = "AI")
	UAISenseConfig_Sight* SightConfig;

	// Функция, которая вызовется САМА, когда враг кого-то увидит
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);
};