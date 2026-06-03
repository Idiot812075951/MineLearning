#include "MiningCompanionCharacter.h"

#include "MineLearning/Mining/MiningToolComponent.h"


AMiningCompanionCharacter::AMiningCompanionCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	MiningToolComponent = CreateDefaultSubobject<UMiningToolComponent>(TEXT("MiningToolComponent"));

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AMiningCompanionCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AddMovementInput(GetActorForwardVector(), 1);
}
