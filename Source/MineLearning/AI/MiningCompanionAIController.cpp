#include "MiningCompanionAIController.h"

#include "MiningCompanionCharacter.h"
#include "MiningCompanionTargetingComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/MiningToolComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/ResourceDepot.h"
#include "MineLearning/Mining/ResourcePickup.h"

AMiningCompanionAIController::AMiningCompanionAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	TargetingComponent = CreateDefaultSubobject<UMiningCompanionTargetingComponent>(TEXT("TargetingComponent"));
}

void AMiningCompanionAIController::SetDebugPaused(bool bPaused)
{
#if !UE_BUILD_SHIPPING
	if (bDebugPaused == bPaused)
	{
		return;
	}

	CacheCompanion();

	if (bPaused)
	{
		bDebugPaused = true;
		StopMovement();

		if (Companion)
		{
			UCharacterMovementComponent* MovementComponent = Companion->GetCharacterMovement();
			if (MovementComponent)
			{
				MovementComponent->StopMovementImmediately();
			}

			USkeletalMeshComponent* Mesh = Companion->GetMesh();
			if (Mesh)
			{
				if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
				{
					AnimInstance->StopAllMontages(0.0f);
				}

				if (MovementComponent)
				{
					MovementComponent->DisableMovement();
				}

				Mesh->bPauseAnims = true;
			}
			else if (MovementComponent)
			{
				MovementComponent->DisableMovement();
			}
		}

		CancelTargetPickup();
		RestoreCollectMovementAndRotation();
		TargetOre = nullptr;
		bAligningForDelivery = false;
		State = EMiningCompanionState::Idle;
		return;
	}

	if (Companion)
	{
		if (USkeletalMeshComponent* Mesh = Companion->GetMesh())
		{
			Mesh->bPauseAnims = false;
		}

		if (UCharacterMovementComponent* MovementComponent = Companion->GetCharacterMovement())
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
	}

	TargetOre = nullptr;
	TargetPickup = nullptr;
	bAligningForDelivery = false;
	State = EMiningCompanionState::Idle;
	bDebugPaused = false;
#endif
}

bool AMiningCompanionAIController::IsDebugPaused() const
{
#if !UE_BUILD_SHIPPING
	return bDebugPaused;
#else
	return false;
#endif
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
	if (bDebugPaused)
	{
		return;
	}

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
		if (GetWorld()->GetTimeSeconds() < NextIdleSearchTime)
		{
			break;
		}

		NextIdleSearchTime = GetWorld()->GetTimeSeconds() + IdleSearchInterval;
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
	case EMiningCompanionState::Mining:
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
	if (bDebugPaused)
	{
		return;
	}

	Super::OnMoveCompleted(RequestID, Result);

	if (State == EMiningCompanionState::ReturningToDelivery)
	{
		if (Result.IsSuccess())
		{
			BeginDeliveryAlignment();
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
		&& TargetPickup->Amount > 0
		&& TargetPickup->IsAvailableFor(Companion);
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

	if (TargetingComponent)
	{
		DeliveryDepot = TargetingComponent->FindDeliveryDepot();
	}
}

FTransform AMiningCompanionAIController::GetDeliveryPointTransform() const
{
	return IsValid(DeliveryDepot)
		? DeliveryDepot->GetDeliveryPointWorldTransform()
		: FTransform::Identity;
}

void AMiningCompanionAIController::BeginDeliveryAlignment()
{
	if (!Companion || !IsValid(DeliveryDepot))
	{
		ResetToIdle();
		return;
	}

	StopCompanionMovement();
	bAligningForDelivery = true;
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
	if (State == EMiningCompanionState::Collecting)
	{
		return;
	}

	if (!Companion || !IsTargetPickupValid())
	{
		ResetToIdle();
		return;
	}

	StopCompanionMovement();
	FaceTargetPickup();
	LockCollectMovementAndRotation();
	State = EMiningCompanionState::Collecting;

	if (!PlayActionMontage(CollectMontage))
	{
		ResetToIdle();
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
	LockCollectMovementAndRotation();
	bAligningForDelivery = false;
	State = EMiningCompanionState::Depositing;

	if (!PlayActionMontage(DepositMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MiningAI] DepositMontage is missing or failed to play"));
		ResetToIdle();
	}
}

void AMiningCompanionAIController::OnActionMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (bDebugPaused)
	{
		return;
	}

	if (State == EMiningCompanionState::Collecting && NotifyName == CollectGrabNotifyName)
	{
		if (!IsTargetPickupValid())
		{
			CancelTargetPickup();

			if (USkeletalMeshComponent* Mesh = Companion->GetMesh())
			{
				if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
				{
					AnimInstance->Montage_Stop(0.1f, CollectMontage);
				}
			}
			return;
		}

		if (!TargetPickup->AttachToCollector(Companion->GetMesh(), CollectSocketName))
		{
			CancelTargetPickup();

			if (USkeletalMeshComponent* Mesh = Companion->GetMesh())
			{
				if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
				{
					AnimInstance->Montage_Stop(0.1f, CollectMontage);
				}
			}
			return;
		}
		return;
	}

	if (State == EMiningCompanionState::Collecting && NotifyName == CollectReleaseNotifyName)
	{
		AResourcePickup* Pickup = TargetPickup;
		TryCollectTargetPickup();

		// TryCollect destroys a fully consumed pickup. Any uncollected remainder
		// or failed collection is detached and restored to the world.
		if (IsValid(Pickup))
		{
			Pickup->CancelCollect(Companion);
		}

		TargetPickup = nullptr;
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
		CancelTargetPickup();
		RestoreCollectMovementAndRotation();

		if (bDebugPaused)
		{
			return;
		}

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

	if (bDebugPaused)
	{
		return;
	}

	if (State == EMiningCompanionState::Depositing)
	{
		RestoreCollectMovementAndRotation();
		ResetToIdle();
	}
}

void AMiningCompanionAIController::ResetToIdle()
{
	StopMovement();
	CancelTargetPickup();
	RestoreCollectMovementAndRotation();
	TargetOre = nullptr;
	bAligningForDelivery = false;
	State = EMiningCompanionState::Idle;
	NextIdleSearchTime = 0.0f;
}

bool AMiningCompanionAIController::FindPickup()
{
	if (!Companion || !GetWorld() || IsCarryFull())
	{
		return false;
	}

	AResourcePickup* BestPickup = TargetingComponent
		? TargetingComponent->FindNearestAvailablePickup(Companion, PickupSearchRadius)
		: nullptr;

	if (!BestPickup)
	{
		return false;
	}

	if (!BestPickup->TryReserve(Companion))
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

	AMineableOre* BestOre = TargetingComponent
		? TargetingComponent->FindNearestOre(Companion, SearchRadius)
		: nullptr;

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
		// MoveToActor may complete synchronously and invoke OnMoveCompleted
		// before returning AlreadyAtGoal. Do not enter the action twice.
		if (State == EMiningCompanionState::MoveToOre)
		{
			EnterMiningState();
		}
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
		if (State == EMiningCompanionState::MoveToPickup)
		{
			StartCollectAction();
		}
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

	if (bAligningForDelivery)
	{
		if (!IsValid(DeliveryDepot))
		{
			ResetToIdle();
			return;
		}

		FRotator TargetRotation = GetDeliveryPointTransform().Rotator();
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;

		const FRotator NewRotation = FMath::RInterpConstantTo(
			Companion->GetActorRotation(),
			TargetRotation,
			DeltaSeconds,
			DeliveryRotationSpeed
		);
		SetControlRotation(NewRotation);
		Companion->SetActorRotation(NewRotation);

		const float RemainingYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(
			NewRotation.Yaw,
			TargetRotation.Yaw
		));
		if (RemainingYaw <= DeliveryRotationTolerance)
		{
			SetControlRotation(TargetRotation);
			Companion->SetActorRotation(TargetRotation);
			bAligningForDelivery = false;
			StartDepositAction();
		}
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
	if (State == EMiningCompanionState::Mining)
	{
		return;
	}

	if (!Companion || !IsTargetOreValid())
	{
		ResetToIdle();
		return;
	}

	StopCompanionMovement();
	FaceTargetOre();

	UMiningToolComponent* MiningComponent = Companion->GetMiningToolComponent();
	if (!MiningComponent)
	{
		ResetToIdle();
		return;
	}

	State = EMiningCompanionState::Mining;

	MiningComponent->OnMiningFinished.RemoveDynamic(this, &AMiningCompanionAIController::OnMiningFinished);
	MiningComponent->OnMiningFinished.AddDynamic(this, &AMiningCompanionAIController::OnMiningFinished);

	if (!MiningComponent->StartMiningTarget(TargetOre))
	{
		MiningComponent->OnMiningFinished.RemoveDynamic(this, &AMiningCompanionAIController::OnMiningFinished);
		MiningComponent->CancelMining();
		ResetToIdle();
	}
}

void AMiningCompanionAIController::OnMiningFinished(bool bInterrupted)
{
	if (Companion)
	{
		if (UMiningToolComponent* MiningComponent = Companion->GetMiningToolComponent())
		{
			MiningComponent->OnMiningFinished.RemoveDynamic(this, &AMiningCompanionAIController::OnMiningFinished);
		}
	}

	if (bDebugPaused)
	{
		return;
	}

	ResetToIdle();
}

void AMiningCompanionAIController::RequestReturnToDelivery()
{
	if (!Companion)
	{
		return;
	}

	CancelTargetPickup();
	RestoreCollectMovementAndRotation();
	TargetOre = nullptr;
	bAligningForDelivery = false;
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

	const EPathFollowingRequestResult::Type MoveResult = MoveToLocation(
		GetDeliveryPointTransform().GetLocation(),
		DeliveryAcceptanceRadius,
		true,
		true,
		true,
		true,
		nullptr,
		true
	);

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		if (State == EMiningCompanionState::ReturningToDelivery && !bAligningForDelivery)
		{
			BeginDeliveryAlignment();
		}
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

void AMiningCompanionAIController::FaceTargetPickup()
{
	if (!Companion || !IsTargetPickupValid())
	{
		return;
	}

	FVector Direction = TargetPickup->GetActorLocation() - Companion->GetActorLocation();
	Direction.Z = 0.0f;

	if (!Direction.IsNearlyZero())
	{
		Companion->SetActorRotation(Direction.Rotation());
	}
}

void AMiningCompanionAIController::LockCollectMovementAndRotation()
{
	if (!Companion || bCollectMovementLocked)
	{
		return;
	}

	bCollectPreviousUseControllerRotationYaw = Companion->bUseControllerRotationYaw;
	Companion->bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* MovementComponent = Companion->GetCharacterMovement())
	{
		CollectPreviousMovementMode = MovementComponent->MovementMode;
		CollectPreviousCustomMovementMode = MovementComponent->CustomMovementMode;
		bCollectPreviousOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
		bCollectPreviousUseControllerDesiredRotation = MovementComponent->bUseControllerDesiredRotation;

		MovementComponent->StopMovementImmediately();
		MovementComponent->bOrientRotationToMovement = false;
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->DisableMovement();
	}

	bCollectMovementLocked = true;
}

void AMiningCompanionAIController::RestoreCollectMovementAndRotation()
{
	if (!Companion || !bCollectMovementLocked)
	{
		return;
	}

	Companion->bUseControllerRotationYaw = bCollectPreviousUseControllerRotationYaw;

	if (UCharacterMovementComponent* MovementComponent = Companion->GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = bCollectPreviousOrientRotationToMovement;
		MovementComponent->bUseControllerDesiredRotation = bCollectPreviousUseControllerDesiredRotation;
		MovementComponent->SetMovementMode(CollectPreviousMovementMode, CollectPreviousCustomMovementMode);
	}

	bCollectMovementLocked = false;
}

void AMiningCompanionAIController::CancelTargetPickup()
{
	if (IsValid(TargetPickup))
	{
		TargetPickup->CancelCollect(Companion);
	}

	TargetPickup = nullptr;
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
