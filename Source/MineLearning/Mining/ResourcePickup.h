#pragma once

#include "ItemPickup.h"
#include "MiningTypes.h"
#include "ResourcePickup.generated.h"

class UStaticMesh;

UCLASS()
class MINELEARNING_API AResourcePickup : public AItemPickup
{
	GENERATED_BODY()

public:
	AResourcePickup();

	void InitializeResource(
		EResourceType InType,
		int32 InAmount,
		const TArray<TObjectPtr<UStaticMesh>>& InDropMeshes,
		float InDropMeshScale
	);

	/** Compatibility view for legacy resource callers. ItemStack is the only stored item data. */
	UFUNCTION(BlueprintPure, Category="Mining|Pickup")
	EResourceType GetResourceType() const;
};
