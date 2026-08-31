// Copyright Epic Games, Inc. All Rights Reserved.

#include "MineLearningGameMode.h"
#include "AI/MiningCompanionAIController.h"
#include "MineLearningCharacter.h"
#include "MineLearningPlayerController.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "UObject/ConstructorHelpers.h"

AMineLearningGameMode::AMineLearningGameMode()
{
	PlayerControllerClass = AMineLearningPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/MineLearning/Player/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// Composition root only: the HUD Blueprint owns the UMG lifecycle and presentation wiring.
	static ConstructorHelpers::FClassFinder<AHUD> HUDBPClass(
		TEXT("/Game/MineLearning/UI/Player/BP_MineLearningHUD"));
	if (HUDBPClass.Class != nullptr)
	{
		HUDClass = HUDBPClass.Class;
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
