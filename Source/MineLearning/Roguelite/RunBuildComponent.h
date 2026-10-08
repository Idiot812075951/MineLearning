#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RunContentCatalog.h"
#include "RunBuildComponent.generated.h"

UCLASS()
class MINELEARNING_API URunBuildComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URunBuildComponent();
	void BeginRun(URunContentCatalog* Config, const TSet<FName>& Nodes, FName Identity);
	void EndRun();
	bool CanAcquire(FName Upgrade) const;
	/** Empty when eligible; shared by acquisition and the draft/catalog queries. */
	FText GetAcquisitionBlock(FName Upgrade) const;
	bool Acquire(FName Upgrade);
	bool HasFormPermission(EPlayerTransformationForm Form) const;
	bool AllowsTransformation() const;
	UFUNCTION(BlueprintPure, Category="Run") int32 GetUpgradeRank(FName Id) const;
	UFUNCTION(BlueprintPure, Category="Run") FName GetSummoner() const { return Summoner; }
	UFUNCTION(BlueprintPure, Category="Run") bool IsRunActive() const { return bActive; }
	const TMap<FName, int32>& GetOwnedUpgrades() const { return Owned; }
	const TSet<FName>& GetLockedTalents() const { return LockedTalents; }
	FGuid GetRunId() const { return RunId; }
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnBuildChanged;
private:
	UPROPERTY(Transient) TObjectPtr<URunContentCatalog> Catalog;
	TSet<FName> LockedTalents;
	TSet<FName> AvailableUpgrades;
	TSet<EPlayerTransformationForm> AvailableForms;
	TMap<FName, int32> Owned;
	FName Summoner;
	FGuid RunId;
	bool bActive = false;
};
