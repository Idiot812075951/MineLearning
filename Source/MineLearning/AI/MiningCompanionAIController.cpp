#include "MiningCompanionAIController.h"

#include "MiningCompanionCharacter.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/MiningToolComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/ResourceDepot.h"
#include "MineLearning/Mining/ResourcePickup.h"

AMiningCompanionAIController::AMiningCompanionAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMiningCompanionAIController::BeginPlay()
{
	Super::BeginPlay();

	CacheCompanion();
	FindDeliveryDepot();

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
		if (IsCarryFull())
		{
			RequestReturnToDelivery();
			break;
		}
		if (FindPickup())
		{
			break;
		}
		FindOre();
		break;

	case EMiningCompanionState::MoveToOre:
		UpdateMoveToOre(DeltaSeconds);
		break;

	case EMiningCompanionState::MoveToPickup:
		UpdateMoveToPickup(DeltaSeconds);
		break;

	case EMiningCompanionState::Collecting:
	case EMiningCompanionState::Depositing:
		break;

	case EMiningCompanionState::Mining:
		UpdateMining(DeltaSeconds);
		break;

	case EMiningCompanionState::ReturningToDelivery:
		UpdateReturningToDelivery(DeltaSeconds);
		break;

	default:
		break;
	}
}

void AMiningCompanionAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (State == EMiningCompanionState::ReturningToDelivery)
	{
		if (Result.IsSuccess())
		{
			StartDepositAction();
		}
		else
		{
			RequestReturnToDelivery();
		}
		return;
	}

	if (State == EMiningCompanionState::MoveToPickup)
	{
		if (Result.IsSuccess())
		{
			StartCollectAction();
			return;
		}

		ResetToIdle();
		return;
	}

	if (State != EMiningCompanionState::MoveToOre)
	{
		return;
	}

	if (!IsTargetOreValid())
	{
		ResetToIdle();
		return;
	}

	if (Result.IsSuccess())
	{
		EnterMiningState();
		return;
	}

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

bool AMiningCompanionAIController::IsTargetPickupValid() const
{
	return IsValid(TargetPickup)
		&& !TargetPickup->IsActorBeingDestroyed()
		&& TargetPickup->Amount > 0;
}

UResourceCarryComponent* AMiningCompanionAIController::GetCarryComponent() const
{
	return Companion ? Companion->GetResourceCarryComponent() : nullptr;
}

bool AMiningCompanionAIController::IsCarryFull() const
{
	const UResourceCarryComponent* CarryComponent = GetCarryComponent();
	return CarryComponent && CarryComponent->IsFull();
}

void AMiningCompanionAIController::FindDeliveryDepot()
{
	if (IsValid(DeliveryDepot) || !GetWorld())
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AResourceDepot::StaticClass(),
		FoundActors
	);

	for (AActor* Actor : FoundActors)
	{
		AResourceDepot* Depot = Cast<AResourceDepot>(Actor);
		if (IsValid(Depot) && !Depot->IsActorBeingDestroyed())
		{
			DeliveryDepot = Depot;
			return;
		}
	}
}

bool AMiningCompanionAIController::TryCollectTargetPickup()
{
	if (!Companion || !IsTargetPickupValid())
	{
		return false;
	}

	return TargetPickup->TryCollect(Companion);
}

bool AMiningCompanionAIController::PlayActionMontage(UAnimMontage* Montage)
{
	if (!Companion || !Montage)
	{
		return false;
	}

	USkeletalMeshComponent* Mesh = Companion->GetMesh();
	if (!Mesh)
	{
		return false;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return false;
	}

	AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &AMiningCompanionAIController::OnActionMontageNotifyBegin);
	AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &AMiningCompanionAIController::OnActionMontageNotifyBegin);

	const float Duration = AnimInstance->Montage_Play(Montage);
	if (Duration <= 0.0f)
	{
		AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &AMiningCompanionAIController::OnActionMontageNotifyBegin);
		return false;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AMiningCompanionAIController::OnActionMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	return true;
}

void AMiningCompanionAIController::StartCollectAction()
{
	if (!Companion || !IsTargetPickupValid())
	{
		ResetToIdle();
		return;
	}

	StopCompanionMovement();
	State = EMiningCompanionState::Collecting;

	if (!PlayActionMontage(CollectMontage))
	{
		// 兼容路径：没有配置 CollectMontage 时，仍通过 ResourcePickup 自己的 TryCollect 完成拾取。
		TryCollectTargetPickup();

		if (IsCarryFull())
		{
			RequestReturnToDelivery();
		}
		else
		{
			ResetToIdle();
		}
	}
}

void AMiningCompanionAIController::StartDepositAction()
{
	UResourceCarryComponent* CarryComponent = GetCarryComponent();
	if (!Companion || !CarryComponent || CarryComponent->GetCurrentOreCount() <= 0)
	{
		ResetToIdle();
		return;
	}

	StopCompanionMovement();
	State = EMiningCompanionState::Depositing;

	if (!PlayActionMontage(DepositMontage))
	{
		// 兼容路径：没有配置 DepositMontage 时，仍通过 ResourceDepot 完成交付。
		DepositCarriedOre();
		ResetToIdle();
	}
}

void AMiningCompanionAIController::OnActionMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (State == EMiningCompanionState::Collecting && NotifyName == CollectNotifyName)
	{
		TryCollectTargetPickup();
		return;
	}

	if (State == EMiningCompanionState::Depositing && NotifyName == DepositNotifyName)
	{
		DepositCarriedOre();
	}
}

void AMiningCompanionAIController::OnActionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Companion)
	{
		if (USkeletalMeshComponent* Mesh = Companion->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &AMiningCompanionAIController::OnActionMontageNotifyBegin);
			}
		}
	}

	if (State == EMiningCompanionState::Collecting)
	{
		TargetPickup = nullptr;
		if (IsCarryFull())
		{
			RequestReturnToDelivery();
		}
		else
		{
			ResetToIdle();
		}
		return;
	}

	if (State == EMiningCompanionState::Depositing)
	{
		ResetToIdle();
	}
}

void AMiningCompanionAIController::ResetToIdle()
{
	StopMovement();
	TargetOre = nullptr;
	TargetPickup = nullptr;
	State = EMiningCompanionState::Idle;
}

bool AMiningCompanionAIController::FindPickup()
{
	if (!Companion || !GetWorld() || IsCarryFull())
	{
		return false;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AResourcePickup::StaticClass(),
		FoundActors
	);

	AResourcePickup* BestPickup = nullptr;
	float BestDistanceSq = PickupSearchRadius * PickupSearchRadius;

	const FVector MyLocation = Companion->GetActorLocation();

	for (AActor* Actor : FoundActors)
	{
		AResourcePickup* Pickup = Cast<AResourcePickup>(Actor);
		if (!IsValid(Pickup) || Pickup->IsActorBeingDestroyed() || Pickup->Amount <= 0)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(MyLocation, Pickup->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestPickup = Pickup;
		}
	}

	if (!BestPickup)
	{
		return false;
	}

	TargetPickup = BestPickup;
	TargetOre = nullptr;
	State = EMiningCompanionState::MoveToPickup;
	RequestMoveToPickup();
	return true;
}

void AMiningCompanionAIController::FindOre()
{
	if (!Companion || !GetWorld())
	{
		return;
	}

	if (IsCarryFull())
	{
		RequestReturnToDelivery();
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

		const float DistanceSq = FVector::DistSquared(MyLocation, Ore->GetActorLocation());
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
	TargetPickup = nullptr;
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

	if (IsCarryFull())
	{
		RequestReturnToDelivery();
		return;
	}

	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(
		TargetOre,
		MiningInteractRadius,
		true,
		true,
		true,
		nullptr,
		true
	);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		ResetToIdle();
		return;
	}

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		EnterMiningState();
		return;
	}

	State = EMiningCompanionState::MoveToOre;
}

void AMiningCompanionAIController::RequestMoveToPickup()
{
	if (!Companion || !IsTargetPickupValid())
	{
		ResetToIdle();
		return;
	}

	if (IsCarryFull())
	{
		RequestReturnToDelivery();
		return;
	}

	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(
		TargetPickup,
		PickupInteractRadius,
		true,
		true,
		true,
		nullptr,
		true
	);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		ResetToIdle();
		return;
	}

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		StartCollectAction();
		return;
	}

	State = EMiningCompanionState::MoveToPickup;
}

void AMiningCompanionAIController::UpdateMoveToPickup(float DeltaSeconds)
{
	if (!IsTargetPickupValid())
	{
		ResetToIdle();
		return;
	}

	if (IsCarryFull())
	{
		RequestReturnToDelivery();
		return;
	}

	if (GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		ResetToIdle();
	}
}

void AMiningCompanionAIController::UpdateMoveToOre(float DeltaSeconds)
{
	if (!IsTargetOreValid())
	{
		ResetToIdle();
	}
}

void AMiningCompanionAIController::UpdateMining(float DeltaSeconds)
{
	if (!Companion)
	{
		return;
	}

	if (IsCarryFull())
	{
		RequestReturnToDelivery();
		return;
	}

	if (FindPickup())
	{
		return;
	}

	if (!IsTargetOreValid())
	{
		ResetToIdle();
		return;
	}

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

	const bool bStartedMining = MiningComponent->StartMiningTarget(TargetOre);
	if (!bStartedMining)
	{
		return;
	}

	if (IsCarryFull())
	{
		RequestReturnToDelivery();
		return;
	}
}

void AMiningCompanionAIController::UpdateReturningToDelivery(float DeltaSeconds)
{
	if (!Companion)
	{
		return;
	}

	const UResourceCarryComponent* CarryComponent = GetCarryComponent();
	if (!CarryComponent || CarryComponent->GetCurrentOreCount() <= 0)
	{
		ResetToIdle();
		return;
	}

	if (const UCharacterMovementComponent* MovementComponent = Companion->GetCharacterMovement())
	{
		if (MovementComponent->MovementMode == MOVE_None)
		{
			return;
		}
	}

	if (GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		RequestReturnToDelivery();
	}
}

void AMiningCompanionAIController::EnterMiningState()
{
	StopCompanionMovement();
	FaceTargetOre();
	State = EMiningCompanionState::Mining;
}

void AMiningCompanionAIController::RequestReturnToDelivery()
{
	if (!Companion)
	{
		return;
	}

	TargetOre = nullptr;
	TargetPickup = nullptr;
	State = EMiningCompanionState::ReturningToDelivery;

	const UResourceCarryComponent* CarryComponent = GetCarryComponent();
	if (!CarryComponent || CarryComponent->GetCurrentOreCount() <= 0)
	{
		ResetToIdle();
		return;
	}

	FindDeliveryDepot();
	if (!IsValid(DeliveryDepot))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MiningAI] No ResourceDepot found for delivery"));
		return;
	}

	if (const UCharacterMovementComponent* MovementComponent = Companion->GetCharacterMovement())
	{
		if (MovementComponent->MovementMode == MOVE_None)
		{
			return;
		}
	}

	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(
		DeliveryDepot,
		DeliveryAcceptanceRadius,
		true,
		true,
		true,
		nullptr,
		true
	);

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		StartDepositAction();
	}
	else if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MiningAI] MoveTo ResourceDepot failed"));
	}
}

void AMiningCompanionAIController::DepositCarriedOre()
{
	const bool bKeepDepositingState = State == EMiningCompanionState::Depositing;
	UResourceCarryComponent* CarryComponent = GetCarryComponent();
	if (!CarryComponent || CarryComponent->GetCurrentOreCount() <= 0)
	{
		TargetOre = nullptr;
		TargetPickup = nullptr;
		if (!bKeepDepositingState)
		{
			State = EMiningCompanionState::Idle;
		}
		return;
	}

	FindDeliveryDepot();
	if (!IsValid(DeliveryDepot))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MiningAI] Deposit failed: no ResourceDepot"));
		return;
	}

	DeliveryDepot->DepositFromCarry(CarryComponent);
	TargetOre = nullptr;
	TargetPickup = nullptr;
	if (!bKeepDepositingState)
	{
		State = EMiningCompanionState::Idle;
	}
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
