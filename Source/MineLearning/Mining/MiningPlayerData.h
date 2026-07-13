#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MiningPlayerData.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDataChangedSignature);

UCLASS(BlueprintType)
class MINELEARNING_API UMiningPlayerData : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Mining|PlayerData")
	int32 AddProcessedOre(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Mining|PlayerData")
	bool ConsumeProcessedOre(int32 Amount);

	UFUNCTION(BlueprintPure, Category="Mining|PlayerData")
	bool CanUsePopulation(int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category="Mining|PlayerData")
	bool UsePopulation(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Mining|PlayerData")
	int32 ReleasePopulation(int32 Amount);

	UPROPERTY(BlueprintAssignable, Category="Mining|PlayerData")
	FOnPlayerDataChangedSignature OnPlayerDataChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|PlayerData")
	int32 ProcessedOre = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|PlayerData")
	int32 PopulationUsed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|PlayerData")
	int32 PopulationLimit = 3;

private:
	void BroadcastPlayerDataChanged();
};
