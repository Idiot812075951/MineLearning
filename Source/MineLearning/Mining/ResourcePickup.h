#pragma once

#include "ItemPickup.h"
#include "MiningTypes.h"
#include "ResourcePickup.generated.h"

class UStaticMesh;
class UMaterialInterface;

UCLASS()
class MINELEARNING_API AResourcePickup : public AItemPickup
{
	GENERATED_BODY()

public:
	AResourcePickup();

	UPROPERTY(EditAnywhere, Category="Mining|Visual")
	TArray<TObjectPtr<UMaterialInterface>> FinishVariants;

	void InitializeResource(
		EResourceType InType,
		int32 InAmount,
		const TArray<TObjectPtr<UStaticMesh>>& InDropMeshes
	);

	/** Compatibility view for legacy resource callers. ItemStack is the only stored item data. */
	UFUNCTION(BlueprintPure, Category="Mining|Pickup")
	EResourceType GetResourceType() const;
};
