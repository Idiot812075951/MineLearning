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

void AMiningCompanionAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (State != EMiningCompanionState::MoveToOre)
	{
		return;
	}

	if (!IsTargetOreValid())
	{
		ResetToIdle();
		return;
	}

	if (IsCloseEnoughToMine())
	{
		EnterMiningState();
		return;
	}

	// MoveTo 结束但还没到挖矿距离，说明这次路径没有到位。
	// 先回 Idle，下一轮重新找目标，不在这里循环 spam MoveTo。
	ResetToIdle();
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
	return IsValid(TargetOre)
		&& !TargetOre->IsActorBeingDestroyed()
		&& !TargetOre->IsDestroyed();
}

bool AMiningCompanionAIController::IsCloseEnoughToMine() const
{
	if (!Companion || !IsTargetOreValid())
	{
		return false;
	}

	const float Distance2D = FVector::Dist2D(
		Companion->GetActorLocation(),
		TargetOre->GetActorLocation()
	);

	return Distance2D <= MiningStartDistance;
}

void AMiningCompanionAIController::ResetToIdle()
{
	StopMovement();

	TargetOre = nullptr;
	State = EMiningCompanionState::Idle;
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
		if (!IsValid(Ore) || Ore->IsActorBeingDestroyed() || Ore->IsDestroyed())
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

	if (IsCloseEnoughToMine())
	{
		EnterMiningState();
		return;
	}

	State = EMiningCompanionState::MoveToOre;
	RequestMoveToOre();
}

void AMiningCompanionAIController::RequestMoveToOre()
{
	if (!Companion || !IsTargetOreValid())
	{
		ResetToIdle();
		return;
	}

	// 缓存本次移动目标，后面不要反复直接访问 TargetOre。
	AMineableOre* MoveTarget = TargetOre;
	if (!IsValid(MoveTarget) || MoveTarget->IsActorBeingDestroyed() || MoveTarget->IsDestroyed())
	{
		ResetToIdle();
		return;
	}

	const FVector TargetLocation = MoveTarget->GetActorLocation();

	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(
		MoveTarget,
		MiningStartDistance,
		false,  // 不把 Capsule / 目标碰撞半径算进到达距离
		true,   // 使用 NavMesh 寻路
		false,  // 不允许横向滑行
		nullptr,
		false   // 不接受半截路径
	);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			546564,
			1.0f,
			FColor::Red,
			FString::Printf(
				TEXT("MoveResult=%d Ore=%s"),
				static_cast<int32>(MoveResult),
				*TargetLocation.ToString()
			)
		);
	}

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		// 只在当前目标仍然是本次 MoveTarget 时重置，避免误清后续目标。
		if (TargetOre == MoveTarget)
		{
			ResetToIdle();
		}
		return;
	}

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		if (TargetOre == MoveTarget && IsCloseEnoughToMine())
		{
			EnterMiningState();
		}
		else
		{
			ResetToIdle();
		}
		return;
	}

	State = EMiningCompanionState::MoveToOre;
}
void AMiningCompanionAIController::UpdateMoveToOre(float DeltaSeconds)
{
	if (!IsTargetOreValid())
	{
		ResetToIdle();
		return;
	}

	const float Distance2D = Companion
		? FVector::Dist2D(Companion->GetActorLocation(), TargetOre->GetActorLocation())
		: 999999.0f;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1111,
			0.0f,
			FColor::Cyan,
			FString::Printf(
				TEXT("MoveToOre OreDist=%.1f Need<=%.1f MoveStatus=%d"),
				Distance2D,
				MiningStartDistance,
				static_cast<int32>(GetMoveStatus())
			)
		);
	}

	if (IsCloseEnoughToMine())
	{
		EnterMiningState();
	}
}

void AMiningCompanionAIController::UpdateMining(float DeltaSeconds)
{
	if (!Companion)
	{
		return;
	}

	if (!IsTargetOreValid())
	{
		ResetToIdle();
		return;
	}

	if (!IsCloseEnoughToMine())
	{
		State = EMiningCompanionState::MoveToOre;
		RequestMoveToOre();
		return;
	}

	FaceTargetOre();

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (Now - LastMiningTime < MiningInterval)
	{
		return;
	}

	LastMiningTime = Now;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			2222,
			0.0f,
			FColor::Green,
			TEXT("State Mining")
		);
	}

	UMiningToolComponent* MiningComponent = Companion->GetMiningToolComponent();
	if (!MiningComponent)
	{
		return;
	}

	MiningComponent->StartMining();
}

void AMiningCompanionAIController::EnterMiningState()
{
	StopCompanionMovement();
	FaceTargetOre();
	State = EMiningCompanionState::Mining;
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
	StopMovement();

	if (!Companion)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = Companion->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
}