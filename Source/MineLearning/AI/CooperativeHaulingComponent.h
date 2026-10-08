#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "MineLearning/Combat/UnitEffectComponent.h"
#include "CooperativeHaulingComponent.generated.h"

class AHaulerCharacter;
class ASharedCarryTask;

UCLASS(BlueprintType)
class MINELEARNING_API URelayHaulingDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) float SearchRadius = 700.f;
	UPROPERTY(EditAnywhere) float Timeout = 12.f;
	UPROPERTY(EditAnywhere) FUnitEffectRule HandoffBoost;
	bool IsValidConfiguration() const;
};

UCLASS(BlueprintType)
class MINELEARNING_API USharedCarryDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) TSubclassOf<ASharedCarryTask> TaskClass;
	UPROPERTY(EditAnywhere) float CapacityMultiplier = 1.5f;
	UPROPERTY(EditAnywhere) float MinimumLoadFraction = 0.75f;
	UPROPERTY(EditAnywhere) float MoveSpeedMultiplier = 0.9f;
	UPROPERTY(EditAnywhere) float HalfSpacing = 75.f;
	UPROPERTY(EditAnywhere) float SearchRadius = 1200.f;
	UPROPERTY(EditAnywhere) float StallTimeout = 3.f;
	/** Upper size of new warehouse shipments while this ability is equipped. */
	UPROPERTY(EditAnywhere, meta=(ClampMin="1", ClampMax="1000")) int32 DispatchBatchSize = 12;
	bool IsValidConfiguration() const;
};

/** Arbitrates only registered owned workers. Existing hauler jobs retain priority. */
UCLASS()
class MINELEARNING_API UCooperativeHaulingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCooperativeHaulingComponent();
	void Configure(URelayHaulingDefinition* InRelay, USharedCarryDefinition* InPair);
	void RegisterUnit(APawn* Unit);
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void UpdateJobs();
	void ReleaseRelay();
	void TryStartPair();
	UPROPERTY(Transient) TObjectPtr<URelayHaulingDefinition> Relay;
	UPROPERTY(Transient) TObjectPtr<USharedCarryDefinition> Pair;
	TArray<TWeakObjectPtr<AHaulerCharacter>> Workers;
	TArray<TWeakObjectPtr<ASharedCarryTask>> Tasks;
	TWeakObjectPtr<AHaulerCharacter> RelayWorker;
	TWeakObjectPtr<APawn> RelaySource;
	double RelayDeadline = 0.;
	FTimerHandle JobsTimer;
};
