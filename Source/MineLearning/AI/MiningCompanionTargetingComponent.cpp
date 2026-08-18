#include "MiningCompanionTargetingComponent.h"

#include "Kismet/GameplayStatics.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/ResourceDepot.h"
#include "MineLearning/Mining/ResourcePickup.h"

UMiningCompanionTargetingComponent::UMiningCompanionTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AResourcePickup* UMiningCompanionTargetingComponent::FindNearestAvailablePickup(AActor* Searcher, float SearchRadius) const
{
	if (!IsValid(Searcher) || !GetWorld())
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AResourcePickup::StaticClass(), FoundActors);

	AResourcePickup* BestPickup = nullptr;
	float BestDistanceSq = FMath::Square(SearchRadius);
	const FVector SearcherLocation = Searcher->GetActorLocation();
	for (AActor* Actor : FoundActors)
	{
		AResourcePickup* Pickup = Cast<AResourcePickup>(Actor);
		if (!IsValid(Pickup) || Pickup->IsActorBeingDestroyed() || Pickup->Amount <= 0
			|| !Pickup->IsAvailableFor(Searcher))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(SearcherLocation, Pickup->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestPickup = Pickup;
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

AResourceDepot* UMiningCompanionTargetingComponent::FindDeliveryDepot() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AResourceDepot::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		AResourceDepot* Depot = Cast<AResourceDepot>(Actor);
		if (IsValid(Depot) && !Depot->IsActorBeingDestroyed())
		{
			return Depot;
		}
	}

	return nullptr;
}
