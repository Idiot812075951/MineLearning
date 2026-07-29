#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MiningTypes.h"
#include "OreDefinitionDataAsset.generated.h"

class AResourcePickup;

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
};
