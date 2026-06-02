// Copyright Epic Games, Inc. All Rights Reserved.

#include "MineLearningGameMode.h"
#include "MineLearningCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMineLearningGameMode::AMineLearningGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
