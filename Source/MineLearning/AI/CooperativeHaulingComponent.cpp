#include "CooperativeHaulingComponent.h"
#include "SharedCarryTask.h"
#include "HaulerCharacter.h"
#include "HaulerAIController.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

bool URelayHaulingDefinition::IsValidConfiguration() const
{
	return FMath::IsFinite(SearchRadius) && SearchRadius > 0.f && FMath::IsFinite(Timeout) && Timeout > 0.f
		&& HandoffBoost.IsValid() && HandoffBoost.Duration > 0.f;
}

bool USharedCarryDefinition::IsValidConfiguration() const
{
	return TaskClass && FMath::IsFinite(CapacityMultiplier) && CapacityMultiplier >= 1.f && CapacityMultiplier <= 10.f
		&& FMath::IsFinite(MinimumLoadFraction) && MinimumLoadFraction > 0.f && MinimumLoadFraction <= CapacityMultiplier
		&& FMath::IsFinite(MoveSpeedMultiplier) && MoveSpeedMultiplier > 0.f && MoveSpeedMultiplier <= 1.f
		&& FMath::IsFinite(HalfSpacing) && HalfSpacing > 0.f && HalfSpacing <= 500.f
		&& FMath::IsFinite(SearchRadius) && SearchRadius > 0.f && FMath::IsFinite(StallTimeout) && StallTimeout > 0.f
		&& DispatchBatchSize > 0 && DispatchBatchSize <= 1000;
}

UCooperativeHaulingComponent::UCooperativeHaulingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCooperativeHaulingComponent::Configure(URelayHaulingDefinition* InRelay, USharedCarryDefinition* InPair)
{
	if (!GetOwner()->HasAuthority()) { return; }
	if ((InRelay && !InRelay->IsValidConfiguration()) || (InPair && !InPair->IsValidConfiguration())) { return; }
	Relay = InRelay;
	Pair = InPair;
	if (!Relay) { ReleaseRelay(); }
	if (!Pair)
	{
		for (const TWeakObjectPtr<ASharedCarryTask>& Task : Tasks) { if (Task.IsValid()) { Task->Abort(); } }
		Tasks.Reset();
	}
	if (Relay || Pair) { GetWorld()->GetTimerManager().SetTimer(JobsTimer, this, &UCooperativeHaulingComponent::UpdateJobs, 0.1f, true); }
	else { GetWorld()->GetTimerManager().ClearTimer(JobsTimer); }
}

void UCooperativeHaulingComponent::RegisterUnit(APawn* Unit)
{
	if (AHaulerCharacter* Worker = Cast<AHaulerCharacter>(Unit)) { Workers.AddUnique(Worker); }
}

void UCooperativeHaulingComponent::ReleaseRelay()
{
	if (RelayWorker.IsValid())
	{
		if (AHaulerAIController* AI = Cast<AHaulerAIController>(RelayWorker->GetController())) { AI->ReleaseCooperativeWork(GetOwner()); }
	}
	RelayWorker.Reset();
	RelaySource.Reset();
}

void UCooperativeHaulingComponent::UpdateJobs()
{
	Workers.RemoveAll([](const TWeakObjectPtr<AHaulerCharacter>& Worker) { return !Worker.IsValid(); });
	Tasks.RemoveAll([](const TWeakObjectPtr<ASharedCarryTask>& Task) { return !Task.IsValid(); });
	const APlayerController* Player = Cast<APlayerController>(GetOwner());
	APawn* Source = Player ? Player->GetPawn() : nullptr;
	UResourceCarryComponent* PlayerCargo = Source ? Source->FindComponentByClass<UResourceCarryComponent>() : nullptr;
	if (Relay && PlayerCargo && !PlayerCargo->IsEmpty())
	{
		if (RelaySource != Source) { ReleaseRelay(); }
		if (!RelayWorker.IsValid())
		{
			float Nearest = FMath::Square(FMath::Max(0.f, Relay->SearchRadius));
			AHaulerCharacter* Candidate = nullptr;
			for (const TWeakObjectPtr<AHaulerCharacter>& Worker : Workers)
			{
				AHaulerAIController* AI = Cast<AHaulerAIController>(Worker->GetController());
				const float Distance = FVector::DistSquared(Worker->GetActorLocation(), Source->GetActorLocation());
				if (Worker != Source && AI && AI->IsAvailableForCooperation() && Distance < Nearest
					&& Worker->GetResourceCarryComponent()->CanAcceptItem(PlayerCargo->GetCurrentItem())) { Candidate = Worker.Get(); Nearest = Distance; }
			}
			if (Candidate && CastChecked<AHaulerAIController>(Candidate->GetController())->ClaimCooperativeWork(GetOwner()))
			{
				RelayWorker = Candidate;
				RelaySource = Source;
				RelayDeadline = GetWorld()->GetTimeSeconds() + FMath::Max(1.f, Relay->Timeout);
			}
		}
		if (RelayWorker.IsValid())
		{
			AHaulerAIController* AI = Cast<AHaulerAIController>(RelayWorker->GetController());
			const UHealthComponent* Health = RelayWorker->FindComponentByClass<UHealthComponent>();
			const FVector Delta = Source->GetActorLocation() - RelayWorker->GetActorLocation();
			if (!AI || (Health && Health->IsDead()) || AI->GetCooperativeTask() != GetOwner() || GetWorld()->GetTimeSeconds() >= RelayDeadline
				|| Delta.Size() > Relay->SearchRadius * 1.5f) { ReleaseRelay(); }
			else if (Delta.Size() <= RelayWorker->GetInteractionRange())
			{
				if (PlayerCargo->TransferTo(RelayWorker->GetResourceCarryComponent()) > 0)
				{
					if (UUnitEffectComponent* Effects = RelayWorker->FindComponentByClass<UUnitEffectComponent>()) { Effects->ApplyEffect(Relay->HandoffBoost); }
				}
				ReleaseRelay();
				AI->SearchNow();
			}
			else
			{
				const EPathFollowingRequestResult::Type Result = AI->MoveToActor(Source, RelayWorker->GetInteractionRange(), true);
				if (Result == EPathFollowingRequestResult::Failed)
				{
					FVector Goal = Source->GetActorLocation(); Goal.Z = RelayWorker->GetActorLocation().Z;
					FHitResult Hit;
					RelayWorker->SetActorLocation(FMath::VInterpConstantTo(RelayWorker->GetActorLocation(), Goal, 0.1f,
						RelayWorker->GetCharacterMovement()->MaxWalkSpeed), true, &Hit);
					if (Hit.bBlockingHit) { ReleaseRelay(); }
				}
			}
		}
	}
	else { ReleaseRelay(); }
	if (Pair) { TryStartPair(); }
}

void UCooperativeHaulingComponent::TryStartPair()
{
	if (!Pair->TaskClass) { return; }
	TArray<AHaulerCharacter*> Available;
	for (const TWeakObjectPtr<AHaulerCharacter>& Worker : Workers)
	{
		const AHaulerAIController* AI = Cast<AHaulerAIController>(Worker->GetController());
		if (AI && AI->IsAvailableForCooperation()) { Available.Add(Worker.Get()); }
	}
	if (Available.Num() < 2) { return; }
	for (TActorIterator<AItemPickup> It(GetWorld()); It; ++It)
	{
		AItemPickup* Pickup = *It;
		Available.Sort([Pickup](const AHaulerCharacter& Left, const AHaulerCharacter& Right)
		{
			return FVector::DistSquared(Left.GetActorLocation(), Pickup->GetActorLocation()) < FVector::DistSquared(Right.GetActorLocation(), Pickup->GetActorLocation());
		});
		if (!Pickup->IsAvailableFor(Available[0]) || !Available[0]->GetResourceCarryComponent()->CanAcceptItem(Pickup->GetItemStack())
			|| !Available[1]->GetResourceCarryComponent()->CanAcceptItem(Pickup->GetItemStack())) { continue; }
		const int32 Capacity = Available[0]->GetResourceCarryComponent()->GetCapacity() + Available[1]->GetResourceCarryComponent()->GetCapacity();
		if (Pickup->GetAmount() < FMath::CeilToInt(Capacity * Pair->MinimumLoadFraction)
			|| FVector::DistSquared(Pickup->GetActorLocation(), Available[0]->GetActorLocation()) > FMath::Square(Pair->SearchRadius)
			|| FVector::DistSquared(Pickup->GetActorLocation(), Available[1]->GetActorLocation()) > FMath::Square(Pair->SearchRadius)) { continue; }
		FActorSpawnParameters Params; Params.Owner = GetOwner();
		ASharedCarryTask* Task = GetWorld()->SpawnActor<ASharedCarryTask>(Pair->TaskClass, Pickup->GetActorLocation(), FRotator::ZeroRotator, Params);
		if (Task && Task->Start(Available[0], Available[1], Pickup, Pair)) { Tasks.Add(Task); return; }
		if (Task) { Task->Destroy(); }
	}
}

void UCooperativeHaulingComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Configure(nullptr, nullptr);
	Super::EndPlay(Reason);
}
