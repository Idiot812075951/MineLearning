#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourceDepot.generated.h"

class UResourceCarryComponent;
class UResourceStorageComponent;
class USceneComponent;

UCLASS(Blueprintable)
class MINELEARNING_API AResourceDepot : public AActor
{
	GENERATED_BODY()

public:
	AResourceDepot();

	UFUNCTION(BlueprintPure, Category="Mining|Depot")
	UResourceStorageComponent* GetStorageComponent() const { return StorageComponent; }

	UFUNCTION(BlueprintPure, Category="Mining|Depot")
	FTransform GetDeliveryPointWorldTransform() const;

	UFUNCTION(BlueprintCallable, Category="Mining|Depot")
	int32 DepositFromCarry(UResourceCarryComponent* CarryComponent);

private:
	UPROPERTY(VisibleAnywhere, Category="Mining|Depot")
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Depot", meta=(MakeEditWidget, AllowPrivateAccess="true"))
	FTransform DeliveryPoint = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Depot", meta=(AllowPrivateAccess="true"))
	UResourceStorageComponent* StorageComponent = nullptr;
};
