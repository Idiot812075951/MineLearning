#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MiningOreFieldManager.generated.h"

class AMineableOre;
class UOreDefinitionDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOreBatchClearedSignature);

USTRUCT(BlueprintType)
struct FMiningOreBatchEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore Field")
	UOreDefinitionDataAsset* OreDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore Field", meta=(ClampMin="0"))
	int32 Count = 1;
};

UCLASS()
class MINELEARNING_API AMiningOreFieldManager : public AActor
{
	GENERATED_BODY()

public:
	AMiningOreFieldManager();

	UFUNCTION(BlueprintCallable, Category="Mining|Ore Field")
	int32 SpawnBatch();

	UFUNCTION(BlueprintCallable, Category="Mining|Ore Field")
	void ClearCurrentBatch();

	UFUNCTION(BlueprintPure, Category="Mining|Ore Field")
	int32 GetRemainingOreCount() const { return RemainingOreCount; }

	UPROPERTY(BlueprintAssignable, Category="Mining|Ore Field")
	FOnOreBatchClearedSignature OnOreBatchCleared;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore Field")
	bool bSpawnBatchOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore Field")
	TSubclassOf<AMineableOre> OreClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore Field")
	TArray<FMiningOreBatchEntry> BatchEntries;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mining|Ore Field")
	TArray<AMineableOre*> ActiveOres;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mining|Ore Field")
	int32 RemainingOreCount = 0;

private:
	UFUNCTION()
	void HandleOreDepleted(AMineableOre* DepletedOre);

	void CompleteCurrentBatch();

	bool bBatchActive = false;
	bool bBatchClearedBroadcast = false;
	bool bClearingCurrentBatch = false;
};
