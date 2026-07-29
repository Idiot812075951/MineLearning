#pragma once

#include "CoreMinimal.h"
#include "MiningTypes.h"
#include "GameFramework/Actor.h"
#include "MineableOre.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class AResourcePickup;
class AMineableOre;
class UOreDefinitionDataAsset;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOreDepletedSignature, AMineableOre*, DepletedOre);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOreHealthChangedSignature, float, CurrentHealth, float, MaxHealth);

UCLASS()
class MINELEARNING_API AMineableOre : public AActor
{
	GENERATED_BODY()

public:
	AMineableOre();

	UFUNCTION(BlueprintCallable)
	bool ApplyMiningHit(const FMiningHitRequest& Request);

	UFUNCTION(BlueprintCallable, Category="Mining|Ore")
	void SetOreDefinition(UOreDefinitionDataAsset* InOreDefinition);

	UFUNCTION(BlueprintPure, Category="Mining|Ore")
	UOreDefinitionDataAsset* GetOreDefinition() const { return OreDefinition; }

	UFUNCTION(BlueprintPure)
	bool IsDestroyed() const { return CurrentHP <= 0.0f; }

	UFUNCTION(BlueprintPure, Category="Mining|Stats")
	float GetCurrentHealth() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category="Mining|Stats")
	float GetMaxHealth() const { return MaxHP; }

	UPROPERTY(BlueprintAssignable, Category="Mining|Ore")
	FOnOreDepletedSignature OnOreDepleted;

	UPROPERTY(BlueprintAssignable, Category="Mining|Ore")
	FOnOreHealthChangedSignature OnOreHealthChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* OreMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|UI")
	UWidgetComponent* HealthBarWidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore")
	UOreDefinitionDataAsset* OreDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Stats")
	float MaxHP = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Stats")
	float CurrentHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Stats")
	float Hardness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Visual")
	UMaterialInterface* BaseMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Visual")
	EMineableDamageStage CurrentStage = EMineableDamageStage::Full;

private:
	void InitializeStatsFromDefinition();
	void ApplyDamageVisual();
	void UpdateDamageStage();
	void SpawnDropsForTrigger(EOreDropTrigger Trigger, const FVector& DropLocation);
	void HandleDepleted();
	void DestroyOre();
	bool SpawnResourceDropDirect(EResourceType Type, int32 Amount, const FVector& DropLocation);

	bool bHasDepleted = false;
};
