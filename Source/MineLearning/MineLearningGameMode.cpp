// Copyright Epic Games, Inc. All Rights Reserved.

#include "MineLearningGameMode.h"
#include "AI/MiningCompanionAIController.h"
#include "MineLearningCharacter.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AMineLearningGameMode::AMineLearningGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/MineLearning/Player/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AMineLearningGameMode::SetAllMiningCompanionsDebugPaused(bool bPaused)
{
#if !UE_BUILD_SHIPPING
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AMiningCompanionAIController> It(World); It; ++It)
	{
		It->SetDebugPaused(bPaused);
	}
#endif
}
