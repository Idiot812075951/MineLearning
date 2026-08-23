#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemReceiver.h"
#include "ResourceDepot.generated.h"

class UResourceCarryComponent;
class UResourceStorageComponent;
class USceneComponent;

UCLASS(Blueprintable)
class MINELEARNING_API AResourceDepot : public AActor, public IItemReceiver
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

	virtual EItemReceiverType GetItemReceiverType_Implementation() const override;
	virtual bool CanAcceptItem_Implementation(const FItemStack& Item) const override;
	virtual bool AcceptItem_Implementation(const FItemStack& Item) override;

private:
	UPROPERTY(VisibleAnywhere, Category="Mining|Depot")
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Depot", meta=(MakeEditWidget, AllowPrivateAccess="true"))
	FTransform DeliveryPoint = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Depot", meta=(AllowPrivateAccess="true"))
	UResourceStorageComponent* StorageComponent = nullptr;
};
