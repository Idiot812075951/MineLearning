#pragma once

#include "CoreMinimal.h"
#include "MiningTypes.h"
#include "GameFramework/Actor.h"
#include "MineableOre.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class AResourcePickup;

UCLASS()
class MINELEARNING_API AMineableOre : public AActor
{
	GENERATED_BODY()

public:
	AMineableOre();

	UFUNCTION(BlueprintCallable)
	bool ApplyMiningHit(const FMiningHitRequest& Request);

	UFUNCTION(BlueprintPure)
	bool IsDestroyed() const { return CurrentHP <= 0.0f; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* OreMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Stats")
	float MaxHP = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Stats")
	float CurrentHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Stats")
	float Hardness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Drop")
	FResourceDropConfig DropConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Drop")
	TSubclassOf<AResourcePickup> ResourcePickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Drop")
	int32 RemainingResourceAmount = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Visual")
	UMaterialInterface* BaseMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Visual")
	EMineableDamageStage CurrentStage = EMineableDamageStage::Full;

private:
	void ApplyDamageVisual();
	void UpdateDamageStage();
	void TryDropResource(const FVector& DropLocation);
	void DestroyOre();
	void SpawnResourceDropDirect(EResourceType Type, int32 Amount, const FVector& DropLocation);
};