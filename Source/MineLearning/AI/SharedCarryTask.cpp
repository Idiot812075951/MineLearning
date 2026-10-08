#include "SharedCarryTask.h"
#include "CooperativeHaulingComponent.h"
#include "HaulerAIController.h"
#include "HaulerCharacter.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Mining/ItemLogisticsLibrary.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

ASharedCarryTask::ASharedCarryTask()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TaskOrigin")));
	Cargo = CreateDefaultSubobject<UResourceCarryComponent>(TEXT("SharedCargo"));
	Cargo->bAcceptsLogisticsOrders = true;
}

bool ASharedCarryTask::Start(AHaulerCharacter* First, AHaulerCharacter* Second, AItemPickup* Pickup, USharedCarryDefinition* Definition)
{
	if (Phase != ESharedCarryPhase::Complete || !HasAuthority() || !IsValid(First) || !IsValid(Second) || First == Second || !IsValid(Pickup) || !Definition
		|| !Definition->IsValidConfiguration()) { return false; }
	AHaulerAIController* FirstAI = Cast<AHaulerAIController>(First->GetController());
	AHaulerAIController* SecondAI = Cast<AHaulerAIController>(Second->GetController());
	if (!FirstAI || !SecondAI || !FirstAI->IsAvailableForCooperation() || !SecondAI->IsAvailableForCooperation()) { return false; }
	Destination = Pickup->HasUsableExplicitDeliveryTarget() ? Pickup->GetExplicitDeliveryActor()
		: UItemLogisticsLibrary::ResolveDestination(this, Pickup->GetItemStack(), Pickup->GetActorLocation());
	if (!IsValid(Destination) || !Pickup->TryReserve(this)) { return false; }
	if (!FirstAI->ClaimCooperativeWork(this)) { Pickup->ReleaseReservation(this); return false; }
	if (!SecondAI->ClaimCooperativeWork(this)) { FirstAI->ReleaseCooperativeWork(this); Pickup->ReleaseReservation(this); return false; }
	A = First; B = Second; Source = Pickup; Config = Definition;
	DestinationStorage = Pickup->GetExplicitDeliveryStorage();
	DestinationPoint = Pickup->GetExplicitDeliveryPoint();
	const int32 Capacity = FMath::CeilToInt((First->GetResourceCarryComponent()->GetCapacity() + Second->GetResourceCarryComponent()->GetCapacity()) * Config->CapacityMultiplier);
	Cargo->ConfigureAcceptance(Capacity, false, {EItemCategory::Ore, EItemCategory::Currency, EItemCategory::ProcessedMaterial});
	if (UResourceStorageComponent* Storage = Source->GetReservationSourceStorage())
	{
		AccessWarehouse = Cast<AWarehouseDepot>(Storage->GetOwner());
		if (AccessWarehouse) { AccessWarehouse->BeginWorkerAccess(this); }
	}
	FVector Origin = Source->GetActorLocation();
	Origin.Z = (First->GetActorLocation().Z + Second->GetActorLocation().Z) * 0.5f;
	SetActorLocation(Origin);
	const FVector InitialDirection = GetDestinationLocation() - Origin;
	if (!InitialDirection.IsNearlyZero()) { SetActorRotation(FRotator(0.f, InitialDirection.Rotation().Yaw, 0.f)); }
	const FVector Side = GetActorRightVector() * Config->HalfSpacing;
	if (!BuildRoute(First->GetActorLocation(), Origin - Side, FirstRoute)
		|| !BuildRoute(Second->GetActorLocation(), Origin + Side, SecondRoute)
		|| !BuildRoute(Origin, GetDestinationLocation(), DeliveryRoute))
	{
		ReleaseWorkers();
		return false;
	}
	Phase = ESharedCarryPhase::Gathering;
	OnTaskChanged.Broadcast();
	return true;
}

bool ASharedCarryTask::IsWorkerValid(AHaulerCharacter* Unit) const
{
	const AHaulerAIController* AI = IsValid(Unit) ? Cast<AHaulerAIController>(Unit->GetController()) : nullptr;
	const UHealthComponent* Health = IsValid(Unit) ? Unit->FindComponentByClass<UHealthComponent>() : nullptr;
	return AI && (!Health || !Health->IsDead()) && !Unit->IsActorBeingDestroyed() && AI->GetCooperativeTask() == this;
}

bool ASharedCarryTask::MoveWorker(AHaulerCharacter* Unit, FVector Goal, float Speed, float DeltaSeconds)
{
	Goal.Z = Unit->GetActorLocation().Z;
	const FVector Before = Unit->GetActorLocation();
	const FVector Delta = Goal - Before;
	if (Delta.SizeSquared2D() < 4.f) { return true; }
	FHitResult Hit;
	const FVector Step = FMath::VInterpConstantTo(Before, Goal, DeltaSeconds, Speed) - Before;
	Unit->SetActorLocation(Before + Step, true, &Hit);
	if (Hit.bBlockingHit && Unit->GetCharacterMovement()->IsMovingOnGround())
	{
		// Delegate step/ramp collision handling to CharacterMovement; never teleport through geometry.
		if (Unit->GetCharacterMovement()->StepUp(FVector::DownVector, Step * (1.f - Hit.Time), Hit)) { Hit.bBlockingHit = false; }
	}
	if (!Delta.IsNearlyZero()) { Unit->SetActorRotation(FRotator(0.f, Delta.Rotation().Yaw, 0.f)); }
	return !Hit.bBlockingHit;
}

FVector ASharedCarryTask::GetDestinationLocation() const
{
	return IsValid(DestinationPoint) ? DestinationPoint->GetComponentLocation() : UItemLogisticsLibrary::GetReceiverDeliveryLocation(Destination);
}

bool ASharedCarryTask::BuildRoute(const FVector& From, const FVector& Goal, TArray<FVector>& Route) const
{
	Route.Reset();
	if (FVector::DistSquared2D(From, Goal) < 4.f) { Route.Add(Goal); return true; }
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (Navigation && Navigation->GetDefaultNavDataInstance())
	{
		FNavLocation StartOnFloor, GoalOnFloor;
		const FVector ProjectionExtent(100.f, 100.f, 300.f);
		if (!Navigation->ProjectPointToNavigation(From, StartOnFloor, ProjectionExtent)
			|| !Navigation->ProjectPointToNavigation(Goal, GoalOnFloor, ProjectionExtent)) { return false; }
		const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), StartOnFloor.Location, GoalOnFloor.Location, A.Get());
		if (!Path || !Path->IsValid() || Path->IsPartial()) { return false; }
		Route = Path->PathPoints;
	}
	// Test/preview worlds without navigation still use collision sweeps and the stall recovery.
	Route.Add(Goal);
	return true;
}

FVector ASharedCarryTask::NextWaypoint(const FVector& From, TArray<FVector>& Route) const
{
	while (Route.Num() > 1 && FVector::DistSquared2D(From, Route[0]) < FMath::Square(25.f)) { Route.RemoveAt(0); }
	return Route.IsEmpty() ? From : Route[0];
}

void ASharedCarryTask::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || Phase == ESharedCarryPhase::Complete) { return; }
	if (Phase == ESharedCarryPhase::Recovering)
	{
		RecoveryRetry -= DeltaSeconds;
		if (RecoveryRetry <= 0.f) { RecoveryRetry = 1.f; RecoverCargo(); }
		return;
	}
	if (!IsWorkerValid(A.Get()) || !IsWorkerValid(B.Get()) || !IsValid(Destination)) { Abort(); return; }
	const float Speed = FMath::Min(A->GetCharacterMovement()->MaxWalkSpeed, B->GetCharacterMovement()->MaxWalkSpeed) * Config->MoveSpeedMultiplier;
	const FVector Offset = GetActorRightVector() * Config->HalfSpacing;
	if (Phase == ESharedCarryPhase::Gathering)
	{
		if (!IsValid(Source) || !Source->IsAvailableFor(this)) { Abort(); return; }
		const FVector FirstGoal = NextWaypoint(A->GetActorLocation(), FirstRoute);
		const FVector SecondGoal = NextWaypoint(B->GetActorLocation(), SecondRoute);
		const float Before = FVector::Dist2D(A->GetActorLocation(), FirstGoal) + FVector::Dist2D(B->GetActorLocation(), SecondGoal);
		MoveWorker(A.Get(), FirstGoal, Speed, DeltaSeconds);
		MoveWorker(B.Get(), SecondGoal, Speed, DeltaSeconds);
		const float After = FVector::Dist2D(A->GetActorLocation(), GetActorLocation() - Offset)
			+ FVector::Dist2D(B->GetActorLocation(), GetActorLocation() + Offset);
		const float RemainingLeg = FVector::Dist2D(A->GetActorLocation(), FirstGoal) + FVector::Dist2D(B->GetActorLocation(), SecondGoal);
		StalledSeconds = RemainingLeg >= Before - 0.01f ? StalledSeconds + DeltaSeconds : 0.f;
		if (After <= 12.f)
		{
			// One reservation commits directly into the shared cargo, never into each worker.
			if (!Source->TryCollect(this)) { Abort(); return; }
			if (IsValid(Source)) { Source->ReleaseReservation(this); }
			Source = nullptr;
			if (AccessWarehouse) { AccessWarehouse->EndWorkerAccess(this); AccessWarehouse = nullptr; }
			AccessWarehouse = Cast<AWarehouseDepot>(Destination);
			if (AccessWarehouse) { AccessWarehouse->BeginWorkerAccess(this); }
			Phase = ESharedCarryPhase::Delivering;
			StalledSeconds = 0.f;
			OnTaskChanged.Broadcast();
		}
	}
	else
	{
		FVector Goal = GetDestinationLocation(); Goal.Z = GetActorLocation().Z;
		if (FVector::Dist2D(Goal, GetActorLocation()) <= 160.f)
		{
			const FItemStack Item = Cargo->GetCurrentItem();
			const bool bDelivered = IsValid(DestinationStorage) ? DestinationStorage->AddItem(Item)
				: UItemLogisticsLibrary::DeliverItemToReceiver(Destination, Item);
			if (bDelivered)
			{
				Cargo->ClearItems();
				Phase = ESharedCarryPhase::Complete;
				ReleaseWorkers();
				OnTaskChanged.Broadcast();
				Destroy();
			}
			else { DeliveryWait += DeltaSeconds; if (DeliveryWait >= Config->StallTimeout) { Abort(); } }
			return;
		}
		Goal = NextWaypoint(GetActorLocation(), DeliveryRoute);
		Goal.Z = GetActorLocation().Z;
		const FVector Delta = Goal - GetActorLocation();
		const FVector BeforeA = A->GetActorLocation();
		const FVector BeforeB = B->GetActorLocation();
		const FVector Center = FMath::VInterpConstantTo(GetActorLocation(), Goal, DeltaSeconds, Speed);
		// Smooth formation rotation limits the sideways distance travelled on a sharp turn.
		const FRotator Facing = FMath::RInterpConstantTo(GetActorRotation(), FRotator(0.f, Delta.Rotation().Yaw, 0.f), DeltaSeconds, 90.f);
		const FVector Side = Facing.RotateVector(FVector::RightVector) * Config->HalfSpacing;
		const bool bA = MoveWorker(A.Get(), Center - Side, Speed * 1.3f, DeltaSeconds);
		const bool bB = MoveWorker(B.Get(), Center + Side, Speed * 1.3f, DeltaSeconds);
		if (!bA || !bB)
		{
			A->SetActorLocation(BeforeA); B->SetActorLocation(BeforeB);
			StalledSeconds += DeltaSeconds;
		}
		else
		{
			SetActorLocation((A->GetActorLocation() + B->GetActorLocation()) * 0.5f);
			SetActorRotation(Facing);
			A->SetActorRotation(Facing); B->SetActorRotation(Facing);
			StalledSeconds = 0.f;
		}
	}
	if (StalledSeconds >= Config->StallTimeout) { Abort(); }
}

void ASharedCarryTask::ReleaseWorkers()
{
	if (IsValid(Source)) { Source->ReleaseReservation(this); }
	if (IsValid(AccessWarehouse)) { AccessWarehouse->EndWorkerAccess(this); }
	AccessWarehouse = nullptr;
	for (AHaulerCharacter* Worker : {A.Get(), B.Get()})
	{
		if (IsValid(Worker))
		{
			if (AHaulerAIController* AI = Cast<AHaulerAIController>(Worker->GetController())) { AI->ReleaseCooperativeWork(this); }
		}
	}
}

void ASharedCarryTask::Abort()
{
	if (Phase == ESharedCarryPhase::Complete || Phase == ESharedCarryPhase::Recovering) { return; }
	Phase = ESharedCarryPhase::Recovering;
	RecoverCargo();
	ReleaseWorkers();
	OnTaskChanged.Broadcast();
}

void ASharedCarryTask::RecoverCargo()
{
	if (bRecoveringCargo) { return; }
	TGuardValue<bool> Guard(bRecoveringCargo, true);
	for (AHaulerCharacter* Worker : {A.Get(), B.Get()})
	{
		if (IsWorkerValid(Worker)) { Cargo->TransferTo(Worker->GetResourceCarryComponent()); }
	}
	if (!Cargo->IsEmpty()) { Cargo->DropAllItems(GetActorLocation()); }
	// If spawning fails, this task remains the cargo owner and retries; no silent deletion.
	if (Cargo->IsEmpty()) { Destroy(); }
}

void ASharedCarryTask::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Reason == EEndPlayReason::Destroyed && !Cargo->IsEmpty() && !bRecoveringCargo) { RecoverCargo(); }
	ReleaseWorkers();
	Super::EndPlay(Reason);
}
