#include "MiningCompanionCharacter.h"

#include "MineLearning/Mining/MiningToolComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"


AMiningCompanionCharacter::AMiningCompanionCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	MiningToolComponent = CreateDefaultSubobject<UMiningToolComponent>(TEXT("MiningToolComponent"));
	ResourceCarryComponent = CreateDefaultSubobject<UResourceCarryComponent>(TEXT("ResourceCarryComponent"));

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
