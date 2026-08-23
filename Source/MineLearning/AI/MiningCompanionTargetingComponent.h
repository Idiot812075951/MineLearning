#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MiningCompanionTargetingComponent.generated.h"

class AMineableOre;
class AItemPickup;

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

	AItemPickup* FindNearestAvailablePickup(
		AActor* Searcher,
		float SearchRadius,
		AActor* RequiredDestination,
		AActor*& OutDestination) const;
	AMineableOre* FindNearestOre(AActor* Searcher, float SearchRadius) const;
};
