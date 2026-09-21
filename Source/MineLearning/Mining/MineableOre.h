#pragma once

#include "CoreMinimal.h"
#include "MiningTypes.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "GameFramework/Actor.h"
#include "MineableOre.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class AResourcePickup;
class AMineableOre;
class UOreDefinitionDataAsset;
class UResourceHitFeedbackComponent;
class UOreVisualComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOreDepletedSignature, AMineableOre*, DepletedOre);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOreHealthChangedSignature, float, CurrentHealth, float, MaxHealth);

UCLASS()
class MINELEARNING_API AMineableOre : public AActor
{
	GENERATED_BODY()

public:
	AMineableOre();

	UFUNCTION(BlueprintCallable, Category="Mining|Ore")
	void SetOreDefinition(UOreDefinitionDataAsset* InOreDefinition);

	UFUNCTION(BlueprintPure, Category="Mining|Ore")
	UOreDefinitionDataAsset* GetOreDefinition() const { return OreDefinition; }

	UFUNCTION(BlueprintPure)
	bool IsDestroyed() const { return HealthComponent->IsDead(); }

	UFUNCTION(BlueprintPure, Category="Mining|Stats")
	float GetCurrentHealth() const { return HealthComponent->GetHealth(); }

	UFUNCTION(BlueprintPure, Category="Mining|Stats")
	float GetMaxHealth() const { return HealthComponent->GetMaxHealth(); }

	/** Business-owned mining stage: 0 is intact, then increments once per settled break threshold. */
	UFUNCTION(BlueprintPure, Category="Mining|Stage")
	int32 GetCurrentMiningStageIndex() const { return CurrentMiningStageIndex; }

	UFUNCTION(BlueprintPure, Category="Mining|Visual")
	UStaticMeshComponent* GetOreMesh() const { return OreMesh; }

	UPROPERTY(BlueprintAssignable, Category="Mining|Ore")
	FOnOreDepletedSignature OnOreDepleted;

	UPROPERTY(BlueprintAssignable, Category="Mining|Ore")
	FOnOreHealthChangedSignature OnOreHealthChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* OreMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Visual")
	TObjectPtr<UResourceHitFeedbackComponent> HitFeedbackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Visual")
	TObjectPtr<UOreVisualComponent> OreVisualComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore")
	UOreDefinitionDataAsset* OreDefinition = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Visual")
	UMaterialInterface* BaseMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

private:
	UFUNCTION() void HandleDamageResolved(const FCombatDamageRequest& Request, const FCombatDamageResult& Result);
	UFUNCTION() void HandleHealthChanged();
	void InitializeStatsFromDefinition();
	void ApplyDamageVisual();
	void SpawnDropsForTrigger(EOreDropTrigger Trigger, const FVector& DropLocation);
	bool ProcessStageBreaks(float PreviousHealth, const FVector& DropLocation);
	bool UsesStageBreakResourceDrops() const;
	const TArray<float>& GetBreakThresholds() const;
	EResourceType GetStageDropResourceType() const;
	void SpawnStageResourceDrops(const FVector& DropLocation);
	void HandleDepleted();
	void DestroyOre();
	bool SpawnResourceDropDirect(EResourceType Type, int32 Amount, const FVector& DropLocation);

	bool bHasDepleted = false;
	int32 CurrentMiningStageIndex = 0;
	TSet<int32> SettledBreakThresholdIndices;
};
