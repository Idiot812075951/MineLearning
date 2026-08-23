#include "MiningCompanionTargetingComponent.h"

#include "Kismet/GameplayStatics.h"
#include "MineLearning/Mining/ItemLogisticsLibrary.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"

UMiningCompanionTargetingComponent::UMiningCompanionTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AItemPickup* UMiningCompanionTargetingComponent::FindNearestAvailablePickup(
	AActor* Searcher,
	float SearchRadius,
	AActor* RequiredDestination,
	AActor*& OutDestination) const
{
	OutDestination = nullptr;
	if (!IsValid(Searcher) || !GetWorld())
	{
		return nullptr;
	}

	UResourceCarryComponent* CarryComponent = Searcher->FindComponentByClass<UResourceCarryComponent>();
	if (!CarryComponent)
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AItemPickup::StaticClass(), FoundActors);

	AItemPickup* BestPickup = nullptr;
	float BestDistanceSq = FMath::Square(SearchRadius);
	const FVector SearcherLocation = Searcher->GetActorLocation();
	for (AActor* Actor : FoundActors)
	{
		AItemPickup* Pickup = Cast<AItemPickup>(Actor);
		if (!IsValid(Pickup) || Pickup->IsActorBeingDestroyed() || Pickup->GetAmount() <= 0
			|| !Pickup->IsAvailableFor(Searcher)
			|| !CarryComponent->CanAcceptItem(Pickup->GetItemStack()))
		{
			continue;
		}

		AActor* CandidateDestination = UItemLogisticsLibrary::ResolveDestination(
			Searcher,
			Pickup->GetItemStack(),
			SearcherLocation);
		if (!IsValid(CandidateDestination)
			|| (IsValid(RequiredDestination) && CandidateDestination != RequiredDestination))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(SearcherLocation, Pickup->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestPickup = Pickup;
			OutDestination = CandidateDestination;
		}
	}

	return BestPickup;
}

AMineableOre* UMiningCompanionTargetingComponent::FindNearestOre(AActor* Searcher, float SearchRadius) const
{
	if (!IsValid(Searcher) || !GetWorld())
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMineableOre::StaticClass(), FoundActors);

	AMineableOre* BestOre = nullptr;
	float BestDistanceSq = FMath::Square(SearchRadius);
	const FVector SearcherLocation = Searcher->GetActorLocation();
	for (AActor* Actor : FoundActors)
	{
		AMineableOre* Ore = Cast<AMineableOre>(Actor);
		if (!IsValid(Ore) || Ore->IsActorBeingDestroyed() || Ore->IsDestroyed())
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(SearcherLocation, Ore->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestOre = Ore;
		}
	}

	return BestOre;
}
