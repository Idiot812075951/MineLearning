#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MiningTypes.h"
#include "OreDefinitionDataAsset.generated.h"

class AResourcePickup;
class UStaticMesh;

UCLASS(BlueprintType)
class MINELEARNING_API UOreDefinitionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore", meta=(ClampMin="1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore")
	TSubclassOf<AResourcePickup> ResourcePickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore")
	TArray<FOreDropRule> DropRules;

	/** Visual meshes for the resource pickups produced by this ore. One valid mesh is selected uniformly per pickup. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore|Drop Visual")
	TArray<TObjectPtr<UStaticMesh>> DropMeshes;

	/** Per-instance scale applied to the selected drop mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore|Drop Visual", meta=(ClampMin="0.01"))
	float DropMeshScale = 0.3f;

	/** Health ratios at which a stage's fixed resource payout is earned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Stage Drops", meta=(ClampMin="0.0", ClampMax="1.0"))
	TArray<float> BreakThresholds = { 0.8f, 0.6f, 0.4f, 0.2f, 0.0f };

	/** Number of real resource pickups awarded once for each crossed break threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Stage Drops", meta=(ClampMin="0"))
	int32 ResourcePerStage = 2;
};
