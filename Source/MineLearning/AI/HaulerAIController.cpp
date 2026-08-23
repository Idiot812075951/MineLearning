#include "HaulerAIController.h"

#include "HaulerCharacter.h"
#include "EngineUtils.h"
#include "MineLearning/Mining/ItemLogisticsLibrary.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Mining/ItemReceiver.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/ResourceDepot.h"
#include "TimerManager.h"

AHaulerAIController::AHaulerAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AHaulerAIController::BeginPlay()
{
	Super::BeginPlay();
	CacheHauler();
	GetWorldTimerManager().SetTimer(
		SearchTimerHandle,
		this,
		&AHaulerAIController::TryFindWork,
		SearchInterval,
		true,
		0.2f);
}

void AHaulerAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SearchTimerHandle);
	if (IsValid(TargetPickup))
	{
		TargetPickup->ReleaseReservation(Hauler);
	}
	Super::EndPlay(EndPlayReason);
}

void AHaulerAIController::SearchNow()
{
	TryFindWork();
}

void AHaulerAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDirectMove
		&& (State == EHaulerState::MovingToPickup || State == EHaulerState::MovingToDestination))
	{
		if (Hauler && Hauler->GetVelocity().SizeSquared2D() <= 1.0f)
		{
			NavigationStallSeconds += DeltaSeconds;
			if (NavigationStallSeconds >= NavigationStallTimeout)
			{
				StopMovement();
				bDirectMove = true;
				NavigationStallSeconds = 0.0f;
			}
		}
		else
		{
			NavigationStallSeconds = 0.0f;
		}
	}
	else
	{
		NavigationStallSeconds = 0.0f;
	}

	if (bDirectMove)
	{
		TickDirectMove(DeltaSeconds);
	}
}

void AHaulerAIController::CacheHauler()
{
	if (!Hauler)
	{
		Hauler = Cast<AHaulerCharacter>(GetPawn());
	}
}

void AHaulerAIController::TryFindWork()
{
	CacheHauler();
	if (!Hauler || State != EHaulerState::Idle)
	{
		return;
	}

	UResourceCarryComponent* CarryComponent = Hauler->GetResourceCarryComponent();
	if (!CarryComponent)
	{
		return;
	}

	if (!CarryComponent->IsEmpty())
	{
		ResolveAndMoveToDestination();
		return;
	}

	if (FindNearestValidPickup())
	{
		MoveToCurrentPickup();
	}
}

bool AHaulerAIController::FindNearestValidPickup()
{
	UResourceCarryComponent* CarryComponent = Hauler ? Hauler->GetResourceCarryComponent() : nullptr;
	if (!CarryComponent || !GetWorld())
	{
		return false;
	}

	AItemPickup* BestPickup = nullptr;
	AActor* BestDestination = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (TActorIterator<AItemPickup> It(GetWorld()); It; ++It)
	{
		AItemPickup* Pickup = *It;
		if (!IsValid(Pickup) || Pickup->IsActorBeingDestroyed()
			|| !Pickup->IsAvailableFor(Hauler)
			|| !CarryComponent->CanAcceptItem(Pickup->GetItemStack()))
		{
			continue;
		}

		AActor* Destination = UItemLogisticsLibrary::ResolveDestination(
			Hauler,
			Pickup->GetItemStack(),
			Pickup->GetActorLocation());
		if (!IsValid(Destination))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(
			Hauler->GetActorLocation(),
			Pickup->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestPickup = Pickup;
			BestDestination = Destination;
		}
	}

	if (!BestPickup || !BestPickup->TryReserve(Hauler))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[HaulerV1] No currently valid pickup"));
		return false;
	}

	TargetPickup = BestPickup;
	TargetDestination = BestDestination;
	UE_LOG(LogTemp, Display, TEXT("[HaulerV1] Selected Pickup=%s Destination=%s"),
		*GetNameSafe(TargetPickup), *GetNameSafe(TargetDestination));
	return true;
}

bool AHaulerAIController::MoveToCurrentPickup()
{
	if (!Hauler || !IsValid(TargetPickup))
	{
		ResetToIdle();
		return false;
	}

	State = EHaulerState::MovingToPickup;
	const FVector PickupLocation = TargetPickup->GetActorLocation();
	const FVector HaulerLocation = Hauler->GetActorLocation();
	bIssuingMoveRequest = true;
	const EPathFollowingRequestResult::Type Result = MoveToActor(
		TargetPickup,
		PickupAcceptanceRadius,
		true,
		true,
		true,
		nullptr,
		true);
	bIssuingMoveRequest = false;
	UE_LOG(LogTemp, Display, TEXT("[HaulerV1] MoveToPickup Result=%d From=%s To=%s"),
		static_cast<int32>(Result),
		*HaulerLocation.ToCompactString(),
		*PickupLocation.ToCompactString());

	if (Result == EPathFollowingRequestResult::Failed)
	{
		bDirectMove = true;
		return true;
	}
	if (Result == EPathFollowingRequestResult::AlreadyAtGoal
		&& State == EHaulerState::MovingToPickup)
	{
		return BeginPickupAnimation();
	}
	return true;
}

bool AHaulerAIController::BeginPickupAnimation()
{
	if (!Hauler || !IsValid(TargetPickup))
	{
		ResetToIdle();
		return false;
	}

	StopMovement();
	bDirectMove = false;
	bPickupCommitted = false;
	State = EHaulerState::PickingUp;
	Hauler->PlayPickupAnimation();
	return true;
}

bool AHaulerAIController::CollectCurrentPickup()
{
	UResourceCarryComponent* CarryComponent = Hauler ? Hauler->GetResourceCarryComponent() : nullptr;
	if (!CarryComponent || !IsValid(TargetPickup)
		|| !TargetPickup->IsAvailableFor(Hauler)
		|| !CarryComponent->CanAcceptItem(TargetPickup->GetItemStack()))
	{
		ResetToIdle();
		return false;
	}

	AActor* CurrentDestination = UItemLogisticsLibrary::ResolveDestination(
		Hauler,
		TargetPickup->GetItemStack(),
		TargetPickup->GetActorLocation());
	if (!IsValid(CurrentDestination))
	{
		ResetToIdle();
		return false;
	}

	const FItemStack PickupItem = TargetPickup->GetItemStack();
	UStaticMesh* PickupMesh = TargetPickup->SelectedDropMesh;
	AItemPickup* CollectedPickup = TargetPickup;
	if (!CollectedPickup->TryCollect(Hauler))
	{
		ResetToIdle();
		return false;
	}
	if (IsValid(CollectedPickup))
	{
		CollectedPickup->ReleaseReservation(Hauler);
	}

	TargetPickup = nullptr;
	TargetDestination = CurrentDestination;
	Hauler->ShowCarriedItem(PickupMesh);
	UE_LOG(LogTemp, Display, TEXT("[HaulerV1] Picked ItemType=%d Amount=%d"),
		static_cast<int32>(PickupItem.ItemType), CarryComponent->GetCurrentItem().Amount);
	bPickupCommitted = true;
	return true;
}

bool AHaulerAIController::ResolveAndMoveToDestination()
{
	UResourceCarryComponent* CarryComponent = Hauler ? Hauler->GetResourceCarryComponent() : nullptr;
	if (!CarryComponent || CarryComponent->IsEmpty())
	{
		ResetToIdle();
		return false;
	}

	TargetDestination = UItemLogisticsLibrary::ResolveDestination(
		Hauler,
		CarryComponent->GetCurrentItem(),
		Hauler->GetActorLocation());
	if (!IsValid(TargetDestination))
	{
		State = EHaulerState::Idle;
		return false;
	}

	State = EHaulerState::MovingToDestination;
	const FVector DestinationLocation = GetDestinationLocation();
	const FVector HaulerLocation = Hauler->GetActorLocation();
	bIssuingMoveRequest = true;
	const EPathFollowingRequestResult::Type Result = MoveToLocation(
		DestinationLocation,
		DestinationAcceptanceRadius,
		true,
		true,
		true,
		true,
		nullptr,
		true);
	bIssuingMoveRequest = false;
	UE_LOG(LogTemp, Display, TEXT("[HaulerV1] MoveToDestination Result=%d From=%s To=%s"),
		static_cast<int32>(Result),
		*HaulerLocation.ToCompactString(),
		*DestinationLocation.ToCompactString());

	if (Result == EPathFollowingRequestResult::Failed)
	{
		bDirectMove = true;
		return true;
	}
	if (Result == EPathFollowingRequestResult::AlreadyAtGoal
		&& State == EHaulerState::MovingToDestination)
	{
		return BeginDropOffAnimation();
	}
	return true;
}

bool AHaulerAIController::BeginDropOffAnimation()
{
	if (!Hauler || !IsValid(TargetDestination))
	{
		ResetToIdle();
		return false;
	}

	StopMovement();
	bDirectMove = false;
	bDropOffCommitted = false;
	State = EHaulerState::DroppingOff;
	Hauler->PlayDropOffAnimation();
	return true;
}

bool AHaulerAIController::DepositCurrentItem()
{
	UResourceCarryComponent* CarryComponent = Hauler ? Hauler->GetResourceCarryComponent() : nullptr;
	if (!CarryComponent || CarryComponent->IsEmpty())
	{
		ResetToIdle();
		return false;
	}

	const FItemStack CarriedItem = CarryComponent->GetCurrentItem();
	AActor* CurrentDestination = UItemLogisticsLibrary::ResolveDestination(
		Hauler,
		CarriedItem,
		Hauler->GetActorLocation());
	if (!IsValid(CurrentDestination)
		|| !CurrentDestination->GetClass()->ImplementsInterface(UItemReceiver::StaticClass()))
	{
		State = EHaulerState::Idle;
		return false;
	}

	if (CurrentDestination != TargetDestination)
	{
		TargetDestination = CurrentDestination;
		return ResolveAndMoveToDestination();
	}

	if (!IItemReceiver::Execute_AcceptItem(TargetDestination, CarriedItem))
	{
		State = EHaulerState::Idle;
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("[HaulerV1] Delivered ItemType=%d Amount=%d Receiver=%s"),
		static_cast<int32>(CarriedItem.ItemType),
		CarriedItem.Amount,
		*GetNameSafe(TargetDestination));
	CarryComponent->ClearItems();
	Hauler->HideCarriedItem();
	bDropOffCommitted = true;
	return true;
}

void AHaulerAIController::HandlePickupAnimationNotify()
{
	if (State == EHaulerState::PickingUp && !bPickupCommitted)
	{
		CollectCurrentPickup();
	}
}

void AHaulerAIController::HandlePickupAnimationFinished()
{
	if (State != EHaulerState::PickingUp)
	{
		return;
	}
	if (!bPickupCommitted && !CollectCurrentPickup())
	{
		return;
	}
	ResolveAndMoveToDestination();
}

void AHaulerAIController::HandleDropOffAnimationNotify()
{
	if (State == EHaulerState::DroppingOff && !bDropOffCommitted)
	{
		DepositCurrentItem();
	}
}

void AHaulerAIController::HandleDropOffAnimationFinished()
{
	if (State != EHaulerState::DroppingOff)
	{
		return;
	}
	if (!bDropOffCommitted && !DepositCurrentItem())
	{
		return;
	}
	ResetToIdle();
}

void AHaulerAIController::OnMoveCompleted(
	FAIRequestID RequestID,
	const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	if (!Result.IsSuccess())
	{
		if (bIssuingMoveRequest)
		{
			return;
		}

		// A valid request may still fail asynchronously when a pickup or receiver is
		// just outside the generated navmesh. Keep the selected job and use the
		// existing deterministic direct-move fallback instead of repeatedly
		// resolving the same destination from Idle.
		if ((State == EHaulerState::MovingToPickup && IsValid(TargetPickup))
			|| (State == EHaulerState::MovingToDestination && IsValid(TargetDestination)))
		{
			bDirectMove = true;
			return;
		}

		ResetToIdle();
		return;
	}

	if (State == EHaulerState::MovingToPickup)
	{
		BeginPickupAnimation();
	}
	else if (State == EHaulerState::MovingToDestination)
	{
		BeginDropOffAnimation();
	}
}

void AHaulerAIController::ResetToIdle()
{
	StopMovement();
	if (IsValid(TargetPickup))
	{
		TargetPickup->ReleaseReservation(Hauler);
	}
	TargetPickup = nullptr;
	TargetDestination = nullptr;
	State = EHaulerState::Idle;
	bDirectMove = false;
	bPickupCommitted = false;
	bDropOffCommitted = false;
	NavigationStallSeconds = 0.0f;
}

FVector AHaulerAIController::GetDestinationLocation() const
{
	if (const AResourceDepot* Depot = Cast<AResourceDepot>(TargetDestination))
	{
		return Depot->GetDeliveryPointWorldTransform().GetLocation();
	}
	return IsValid(TargetDestination) ? TargetDestination->GetActorLocation() : FVector::ZeroVector;
}

void AHaulerAIController::TickDirectMove(float DeltaSeconds)
{
	if (!Hauler)
	{
		ResetToIdle();
		return;
	}

	FVector TargetLocation = FVector::ZeroVector;
	float AcceptanceRadius = 0.0f;
	if (State == EHaulerState::MovingToPickup && IsValid(TargetPickup))
	{
		TargetLocation = TargetPickup->GetActorLocation();
		AcceptanceRadius = PickupAcceptanceRadius;
	}
	else if (State == EHaulerState::MovingToDestination && IsValid(TargetDestination))
	{
		TargetLocation = GetDestinationLocation();
		AcceptanceRadius = DestinationAcceptanceRadius;
	}
	else
	{
		ResetToIdle();
		return;
	}

	const FVector CurrentLocation = Hauler->GetActorLocation();
	TargetLocation.Z = CurrentLocation.Z;
	const FVector Delta = TargetLocation - CurrentLocation;
	if (Delta.Size2D() <= AcceptanceRadius)
	{
		bDirectMove = false;
		if (State == EHaulerState::MovingToPickup)
		{
			BeginPickupAnimation();
		}
		else
		{
			BeginDropOffAnimation();
		}
		return;
	}

	const FVector NewLocation = FMath::VInterpConstantTo(
		CurrentLocation,
		TargetLocation,
		DeltaSeconds,
		DirectMoveSpeed);
	Hauler->SetActorLocation(NewLocation, false);
	if (!Delta.IsNearlyZero())
	{
		FRotator Facing = Delta.Rotation();
		Facing.Pitch = 0.0f;
		Facing.Roll = 0.0f;
		Hauler->SetActorRotation(Facing);
	}
}
