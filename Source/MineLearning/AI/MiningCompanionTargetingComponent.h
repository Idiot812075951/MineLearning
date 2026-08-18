#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MiningCompanionTargetingComponent.generated.h"

class AMineableOre;
class AResourceDepot;
class AResourcePickup;

/**
 * Read-only world queries used by the mining companion controller.
 * Reservation and state transitions deliberately remain in the controller.
 */
UCLASS(ClassGroup=(Mining), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UMiningCompanionTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMiningCompanionTargetingComponent();

	AResourcePickup* FindNearestAvailablePickup(AActor* Searcher, float SearchRadius) const;
	AMineableOre* FindNearestOre(AActor* Searcher, float SearchRadius) const;
	AResourceDepot* FindDeliveryDepot() const;
};
