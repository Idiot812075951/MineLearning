#include "GunnerAIController.h"

#include "GunnerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MineLearning/Mining/MineableOre.h"

AGunnerAIController::AGunnerAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGunnerAIController::BeginPlay()
{
	Super::BeginPlay();
	CacheGunner();
	State = EGunnerAIState::Idle;
}

void AGunnerAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CacheGunner();
	if (!Gunner)
	{
		return;
	}

	switch (State)
	{
	case EGunnerAIState::Idle:
		FindOre();
		break;
	case EGunnerAIState::MoveToOre:
		if (!IsTargetOreValid())
		{
			ResetToIdle();
		}
		break;
	case EGunnerAIState::Attacking:
		UpdateAttacking(DeltaSeconds);
		break;
	default:
		break;
	}
}

void AGunnerAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	if (State != EGunnerAIState::MoveToOre)
	{
		return;
	}

	if (Result.IsSuccess() && IsTargetOreValid())
	{
		BeginAttacking();
		return;
	}

	ResetToIdle();
}

void AGunnerAIController::CacheGunner()
{
	if (!Gunner)
	{
		Gunner = Cast<AGunnerCharacter>(GetPawn());
	}
}

void AGunnerAIController::FindOre()
{
	if (!Gunner || !GetWorld())
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMineableOre::StaticClass(), FoundActors);

	AMineableOre* BestOre = nullptr;
	float BestDistanceSq = FMath::Square(SearchRadius);
	for (AActor* Actor : FoundActors)
	{
		AMineableOre* Ore = Cast<AMineableOre>(Actor);
		if (!IsValid(Ore) || Ore->IsActorBeingDestroyed() || Ore->IsDestroyed())
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(Gunner->GetActorLocation(), Ore->GetActorLocation());
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
	RequestMoveToOre();
}

void AGunnerAIController::RequestMoveToOre()
{
	if (!Gunner || !IsTargetOreValid())
	{
		ResetToIdle();
		return;
	}

	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(
		TargetOre,
		AttackRange,
		true,
		true,
		true,
		nullptr,
		true);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		ResetToIdle();
		return;
	}

	State = EGunnerAIState::MoveToOre;
	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		BeginAttacking();
	}
}

void AGunnerAIController::BeginAttacking()
{
	if (!Gunner || !IsTargetOreValid())
	{
		ResetToIdle();
		return;
	}

	StopMovement();
	if (UCharacterMovementComponent* Movement = Gunner->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->bOrientRotationToMovement = false;
	}
	State = EGunnerAIState::Attacking;
}

void AGunnerAIController::UpdateAttacking(float DeltaSeconds)
{
	if (!Gunner || !IsTargetOreValid())
	{
		ResetToIdle();
		return;
	}

	const float DistanceSq = FVector::DistSquared2D(Gunner->GetActorLocation(), TargetOre->GetActorLocation());
	if (DistanceSq > FMath::Square(AttackRange * 1.15f))
	{
		if (UCharacterMovementComponent* Movement = Gunner->GetCharacterMovement())
		{
			Movement->bOrientRotationToMovement = true;
		}
		RequestMoveToOre();
		return;
	}

	FaceTarget(DeltaSeconds);
	Gunner->TryFireAtOre(TargetOre);
}

void AGunnerAIController::ResetToIdle()
{
	StopMovement();
	TargetOre = nullptr;
	State = EGunnerAIState::Idle;
	if (Gunner)
	{
		if (UCharacterMovementComponent* Movement = Gunner->GetCharacterMovement())
		{
			Movement->bOrientRotationToMovement = true;
		}
	}
}

bool AGunnerAIController::IsTargetOreValid() const
{
	return IsValid(TargetOre) && !TargetOre->IsActorBeingDestroyed() && !TargetOre->IsDestroyed();
}

void AGunnerAIController::FaceTarget(float DeltaSeconds)
{
	if (!Gunner || !IsTargetOreValid())
	{
		return;
	}

	FVector Direction = TargetOre->GetActorLocation() - Gunner->GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	FRotator Desired = Direction.Rotation();
	Desired.Pitch = 0.0f;
	Desired.Roll = 0.0f;
	const FRotator NewRotation = FMath::RInterpConstantTo(Gunner->GetActorRotation(), Desired, DeltaSeconds, RotationSpeed);
	SetControlRotation(NewRotation);
	Gunner->SetActorRotation(NewRotation);
}
