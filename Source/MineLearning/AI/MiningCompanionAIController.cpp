#include "MiningCompanionAIController.h"

#include "MiningCompanionCharacter.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/MiningToolComponent.h"

AMiningCompanionAIController::AMiningCompanionAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMiningCompanionAIController::BeginPlay()
{
	Super::BeginPlay();

	CacheCompanion();
	State = EMiningCompanionState::Idle;
}

void AMiningCompanionAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CacheCompanion();

	if (!Companion)
	{
		return;
	}

	switch (State)
	{
	case EMiningCompanionState::Idle:
		FindOre();
		break;

	case EMiningCompanionState::MoveToOre:
		UpdateMoveToOre(DeltaSeconds);
		break;

	case EMiningCompanionState::Mining:
		UpdateMining(DeltaSeconds);
		break;

	default:
		break;
	}
}

void AMiningCompanionAIController::CacheCompanion()
{
	if (Companion)
	{
		return;
	}

	Companion = Cast<AMiningCompanionCharacter>(GetPawn());
}

bool AMiningCompanionAIController::IsTargetOreValid() const
{
	return IsValid(TargetOre) && !TargetOre->IsActorBeingDestroyed();
}

void AMiningCompanionAIController::FindOre()
{
	if (!Companion || !GetWorld())
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AMineableOre::StaticClass(),
		FoundActors
	);

	AMineableOre* BestOre = nullptr;
	float BestDistanceSq = SearchRadius * SearchRadius;

	const FVector MyLocation = Companion->GetActorLocation();

	for (AActor* Actor : FoundActors)
	{
		AMineableOre* Ore = Cast<AMineableOre>(Actor);
		if (!IsValid(Ore) || Ore->IsActorBeingDestroyed())
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(
			MyLocation,
			Ore->GetActorLocation()
		);

		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestOre = Ore;
		}
	}

	if (!BestOre)
	{
		return;
	}

	TargetOre = BestOre;
	State = EMiningCompanionState::MoveToOre;
}

void AMiningCompanionAIController::UpdateMoveToOre(float DeltaSeconds)
{
	if (!Companion)
	{
		return;
	}

	if (!IsTargetOreValid())
	{
		TargetOre = nullptr;
		State = EMiningCompanionState::Idle;
		return;
	}

	const float Distance = FVector::Dist(
		Companion->GetActorLocation(),
		TargetOre->GetActorLocation()
	);

	if (Distance <= MiningDistance)
	{
		StopCompanionMovement();
		FaceTargetOre();

		State = EMiningCompanionState::Mining;
		return;
	}

	FVector Direction = TargetOre->GetActorLocation() - Companion->GetActorLocation();
	Direction.Z = 0.0f;

	if (Direction.IsNearlyZero())
	{
		return;
	}

	Direction.Normalize();

	Companion->SetActorRotation(Direction.Rotation());
	Companion->AddMovementInput(Direction, MoveInputScale);
}

void AMiningCompanionAIController::UpdateMining(float DeltaSeconds)
{
	if (!Companion)
	{
		return;
	}

	if (!IsTargetOreValid())
	{
		TargetOre = nullptr;
		State = EMiningCompanionState::Idle;
		return;
	}

	const float Distance = FVector::Dist(
		Companion->GetActorLocation(),
		TargetOre->GetActorLocation()
	);

	if (Distance > MiningDistance * 1.3f)
	{
		State = EMiningCompanionState::MoveToOre;
		return;
	}

	StopCompanionMovement();
	FaceTargetOre();

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (Now - LastMiningTime < MiningInterval)
	{
		return;
	}

	LastMiningTime = Now;

	UMiningToolComponent* MiningComponent = Companion->GetMiningToolComponent();
	if (!MiningComponent)
	{
		return;
	}

	MiningComponent->StartMining();
}

void AMiningCompanionAIController::FaceTargetOre()
{
	if (!Companion || !IsTargetOreValid())
	{
		return;
	}

	FVector Direction = TargetOre->GetActorLocation() - Companion->GetActorLocation();
	Direction.Z = 0.0f;

	if (Direction.IsNearlyZero())
	{
		return;
	}

	Direction.Normalize();
	Companion->SetActorRotation(Direction.Rotation());
}

void AMiningCompanionAIController::StopCompanionMovement()
{
	if (!Companion)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = Companion->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
}