#include "MiningCompanionCharacter.h"

#include "Components/CapsuleComponent.h"
#include "MineLearning/Mining/MiningToolComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"


AMiningCompanionCharacter::AMiningCompanionCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	MiningToolComponent = CreateDefaultSubobject<UMiningToolComponent>(TEXT("MiningToolComponent"));
	ResourceCarryComponent = CreateDefaultSubobject<UResourceCarryComponent>(TEXT("ResourceCarryComponent"));
	ResourceCarryComponent->ConfigureAcceptance(4, false, {EItemCategory::Ore});

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AMiningCompanionCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);
}
