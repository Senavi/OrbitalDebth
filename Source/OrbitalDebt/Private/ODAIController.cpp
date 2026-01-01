#include "ODAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ODCharacter.h" // Чтобы мы могли отличить Игрока от стены
#include "ODEnemy.h"

AODAIController::AODAIController()
{
	// 1. Создаем компонент восприятия
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	
	// 2. Создаем конфиг зрения
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	// 3. Настраиваем параметры
	SightConfig->SightRadius = 1500.0f; // Видит на 15 метров
	SightConfig->LoseSightRadius = 2000.0f; // Теряет из виду на 20 метрах
	SightConfig->PeripheralVisionAngleDegrees = 90.0f; // Угол обзора (как у человека)
	
	// Что именно мы хотим замечать? (Врагов, друзей, нейтралов)
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	// Применяем конфиг к компоненту
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AODAIController::BeginPlay()
{
	Super::BeginPlay();

	// Подписываемся на событие "Кто-то попал в поле зрения"
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AODAIController::OnTargetDetected);
	}
}

void AODAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AODAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	// Stimulus.WasSuccessfullySensed() = true, если мы УВИДЕЛИ. false, если ПОТЕРЯЛИ из виду.
	
	// Проверяем, что это Игрок (Cast to ODCharacter)
	if (auto* PlayerCharacter = Cast<AODCharacter>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			// УВИДЕЛ -> Записываем в память
			GetBlackboardComponent()->SetValueAsObject(TEXT("TargetActor"), PlayerCharacter);
			UE_LOG(LogTemp, Warning, TEXT("I SEE YOU!"));
		}
		else
		{
			// ПОТЕРЯЛ -> Стираем из памяти (через 3 секунды он забудет, но пока стираем сразу для теста)
			// GetBlackboardComponent()->ClearValue(TEXT("TargetActor")); 
			// Пока оставим, чтобы он бежал до последнего места, где видел
		}
	}
}