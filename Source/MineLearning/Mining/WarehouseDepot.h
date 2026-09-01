#pragma once

#include "CoreMinimal.h"
#include "ResourceDepot.h"
#include "WarehouseDepot.generated.h"

class AHaulerCharacter;
class AItemPickup;
class AOreProcessorMachine;
class APawn;
class ASellStation;
class UBoxComponent;
class UInstancedStaticMeshComponent;
class UPrimitiveComponent;
class UResourceStorageComponent;
class USceneComponent;
class UStaticMesh;
class UTexture2D;

USTRUCT(BlueprintType)
struct MINELEARNING_API FWarehouseItemViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Warehouse")
	EItemType ItemType = EItemType::IronOre;

	UPROPERTY(BlueprintReadOnly, Category="Warehouse")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category="Warehouse")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, Category="Warehouse")
	int32 TotalAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Warehouse")
	int32 AvailableAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Warehouse")
	int32 FrozenAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Warehouse")
	int32 UnitSellPrice = 0;

	UPROPERTY(BlueprintReadOnly, Category="Warehouse")
	FText DeliveryDestination;

	UPROPERTY(BlueprintReadOnly, Category="Warehouse")
	bool bCanSell = false;

	UPROPERTY(BlueprintReadOnly, Category="Warehouse")
	bool bCanProcess = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWarehouseChangedSignature);

/**
 * Runtime warehouse implementation kept in C++ so BP_Warehouse stays an
 * editable art assembly instead of accumulating debug choreography nodes.
 */
UCLASS(Blueprintable, HideCategories=(Mining))
class MINELEARNING_API AWarehouseDepot : public AResourceDepot
{
	GENERATED_BODY()

public:
	AWarehouseDepot();

	UFUNCTION(BlueprintCallable, Category="Warehouse|Door")
	void OpenWarehouse();

	UFUNCTION(BlueprintCallable, Category="Warehouse|Door")
	void CloseWarehouse();

	UFUNCTION(BlueprintPure, Category="Warehouse|Interaction")
	bool IsPlayerInInteractionRange(const APawn* PlayerPawn) const;

	UFUNCTION(BlueprintPure, Category="Warehouse|Inventory")
	TArray<FWarehouseItemViewData> GetInventoryViewData() const;

	UFUNCTION(BlueprintCallable, Category="Warehouse|Orders")
	bool RequestSell(EItemType ItemType, int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Warehouse|Orders")
	bool RequestProcess(EItemType ItemType, int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Warehouse|Orders")
	int32 CancelPendingOrder(EItemType ItemType, int32 Amount);

	virtual bool AcceptItem_Implementation(const FItemStack& Item) override;

	UPROPERTY(BlueprintAssignable, Category="Warehouse")
	FOnWarehouseChangedSignature OnWarehouseChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category="Warehouse|Interaction")
	TObjectPtr<UBoxComponent> WorkerInteractionTrigger;

	UPROPERTY(VisibleAnywhere, Category="Warehouse|Inventory")
	TObjectPtr<UInstancedStaticMeshComponent> InventoryCoinVisual;

	UPROPERTY(VisibleAnywhere, Category="Warehouse|Inventory")
	TObjectPtr<UInstancedStaticMeshComponent> InventoryOreVisual;

	UPROPERTY(VisibleAnywhere, Category="Warehouse|Inventory")
	TObjectPtr<UInstancedStaticMeshComponent> InventoryIngotVisual;

	UPROPERTY(EditAnywhere, Category="Warehouse|Door", meta=(ClampMin="1.0"))
	float DoorRotationSpeed = 180.0f;

	UPROPERTY(EditAnywhere, Category="Warehouse|Door")
	float DoorOpenRoll = 90.0f;

	UPROPERTY(EditAnywhere, Category="Warehouse|Inventory", meta=(ClampMin="1", ClampMax="24"))
	int32 MaxVisibleItemsPerType = 12;

	UPROPERTY(EditAnywhere, Category="Warehouse|Interaction", meta=(ClampMin="50.0"))
	float PlayerInteractionRange = 360.0f;

	UPROPERTY(EditDefaultsOnly, Category="Warehouse|Orders")
	TObjectPtr<UStaticMesh> OutboundOreMesh;

	UPROPERTY(EditDefaultsOnly, Category="Warehouse|Inventory")
	TObjectPtr<UStaticMesh> IronIngotMesh;

	UPROPERTY()
	TObjectPtr<AHaulerCharacter> ActiveWorker;

	UPROPERTY()
	TObjectPtr<USceneComponent> DoorPivotComponent;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> BarrierVisualComponent;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> DoorSafetyBlockerComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AItemPickup>> PendingOrderPickups;

	bool bDoorOpenRequested = false;
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
	void AddInventoryInstances(
		UInstancedStaticMeshComponent* Visual,
		int32 Count,
		float GroupWorldOffsetY,
		float YawStepDegrees);
	void UpdateDoorFeedback(bool bFullyOpen);
	void TryCloseAfterDelivery();
	ASellStation* FindSellStation(const FItemStack& Item) const;
	AOreProcessorMachine* FindProcessor() const;
	bool RequestDeliveryOrder(
		const FItemStack& Item,
		AActor* Destination,
		USceneComponent* DestinationPoint);
	void CleanupOrderPickups();
	void BroadcastWarehouseChanged();
	FVector ResolveOutboundPickupLocation(const USceneComponent* DockPoint) const;
	USceneComponent* FindSceneComponent(FName ComponentName) const;
};
