#pragma once

#include "CoreMinimal.h"
#include "ResourceDepot.h"
#include "WarehouseDepot.generated.h"

class AHaulerCharacter;
class UBoxComponent;
class UInstancedStaticMeshComponent;
class UPrimitiveComponent;
class UResourceStorageComponent;
class USceneComponent;

/**
 * Runtime warehouse implementation kept in C++ so BP_Warehouse stays an
 * editable art assembly instead of accumulating debug choreography nodes.
 */
UCLASS(Blueprintable)
class MINELEARNING_API AWarehouseDepot : public AResourceDepot
{
	GENERATED_BODY()

public:
	AWarehouseDepot();

	UFUNCTION(BlueprintCallable, Category="Warehouse|Door")
	void OpenWarehouse();

	UFUNCTION(BlueprintCallable, Category="Warehouse|Door")
	void CloseWarehouse();

	/**
	 * Temporary test hook that emits every complete two-Coin batch at the
	 * Warehouse dock and assigns Robot Center PaymentDropPoint as its explicit
	 * Hauler destination. Returns the number of Coins dispatched.
	 *
	 * TODO: Replace this hook with formal Warehouse outbound orders.
	 */
	UFUNCTION(BlueprintCallable, Category="Warehouse|Debug", meta=(DevelopmentOnly))
	int32 DispatchCoinBatchesToRobotCenterForTesting();

	virtual bool AcceptItem_Implementation(const FItemStack& Item) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category="Warehouse|Interaction")
	TObjectPtr<UBoxComponent> WorkerInteractionTrigger;

	UPROPERTY(VisibleAnywhere, Category="Warehouse|Inventory")
	TObjectPtr<UInstancedStaticMeshComponent> InventoryCoinVisual;

	UPROPERTY(EditAnywhere, Category="Warehouse|Door", meta=(ClampMin="1.0"))
	float DoorRotationSpeed = 180.0f;

	UPROPERTY(EditAnywhere, Category="Warehouse|Door")
	float DoorOpenRoll = 90.0f;

	UPROPERTY(EditAnywhere, Category="Warehouse|Inventory", meta=(ClampMin="1", ClampMax="24"))
	int32 MaxVisibleCoins = 12;

	UPROPERTY()
	TObjectPtr<AHaulerCharacter> ActiveWorker;

	UPROPERTY()
	TObjectPtr<USceneComponent> DoorPivotComponent;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> BarrierVisualComponent;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> DoorSafetyBlockerComponent;

	bool bDoorOpenRequested = false;
	bool bTemporaryCoinDispatchInProgress = false;
	float CurrentDoorRoll = 0.0f;
	FTimerHandle CloseDoorTimer;

	UFUNCTION()
	void HandleWorkerEnter(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleWorkerExit(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	UFUNCTION()
	void HandleStorageChanged(int32 StoredOreCount);

	void CacheAuthoredComponents();
	void RefreshInventoryVisual();
	void UpdateDoorFeedback(bool bFullyOpen);
	void TryCloseAfterDelivery();
	bool FindTemporaryRobotCenterDeliveryTarget(
		UResourceStorageComponent*& OutStorage,
		USceneComponent*& OutDropPoint) const;
	FVector ResolveOutboundPickupLocation(const USceneComponent* DockPoint) const;
	USceneComponent* FindSceneComponent(FName ComponentName) const;
};
