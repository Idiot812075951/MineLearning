#include "MiningCompanionCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MineLearning/Mining/MiningToolComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "OreBuddyAnimInstance.h"


AMiningCompanionCharacter::AMiningCompanionCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	MiningToolComponent = CreateDefaultSubobject<UMiningToolComponent>(TEXT("MiningToolComponent"));
	MiningToolComponent->SetMiningHitSocketName(TEXT("S_DrillTip"));
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

	if (MiningToolComponent)
	{
		// Older placed OreBuddy instances may still serialize the player's legacy
		// PickaxeRightSocket override. Normalize after instance deserialization.
		MiningToolComponent->SetMiningHitSocketName(TEXT("S_DrillTip"));
		MiningToolComponent->OnMiningHitConfirmed.RemoveDynamic(this, &AMiningCompanionCharacter::HandleMiningHitConfirmed);
		MiningToolComponent->OnMiningHitConfirmed.AddDynamic(this, &AMiningCompanionCharacter::HandleMiningHitConfirmed);
	}
}

void AMiningCompanionCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MiningToolComponent)
	{
		MiningToolComponent->OnMiningHitConfirmed.RemoveDynamic(this, &AMiningCompanionCharacter::HandleMiningHitConfirmed);
	}

	Super::EndPlay(EndPlayReason);
}

void AMiningCompanionCharacter::HandleMiningHitConfirmed(FVector HitLocation, FVector HitNormal)
{
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (UOreBuddyAnimInstance* OreBuddyAnimInstance = Cast<UOreBuddyAnimInstance>(CharacterMesh->GetAnimInstance()))
		{
			OreBuddyAnimInstance->TriggerMiningImpact();
		}
	}

	if (MiningImpactSystem && GetWorld())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			MiningImpactSystem,
			HitLocation,
			HitNormal.Rotation(),
			FVector::OneVector,
			true,
			true
		);
	}

	if (MiningImpactSound && GetWorld())
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), MiningImpactSound, HitLocation);
	}
}
