#pragma once

#include "CoreMinimal.h"
#include "ResourceProcessor.h"
#include "OreProcessorMachine.generated.h"

class AItemPickup;
class USceneComponent;
class USplineComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EOreProcessorMachineState : uint8
{
	Idle,
	TransportingInput,
	WaitingForQueue,
	Processing,
	TransportingOutput
};

USTRUCT()
struct FOreProcessorTransportItem
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AItemPickup> Pickup = nullptr;

	UPROPERTY()
	FItemStack Stack;

	UPROPERTY()
	float Distance = 0.0f;

	/** Absolute world time when this item may leave the receiving bin. */
	UPROPERTY()
	float ReleaseTimeSeconds = 0.0f;

	UPROPERTY()
	bool bWaitingAtInput = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProcessorQueueChanged, int32, Occupied, int32, Capacity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOreProcessorStateChanged, EOreProcessorMachineState, NewState);

/**
 * Production OreProcessor flow:
 * real ore pickup -> authored InputPoint parking/delivery point -> input spline visual entry
 * -> capacity-one processing queue
 * -> output spline -> authored OutputPoint.
 */
UCLASS(Blueprintable)
class MINELEARNING_API AOreProcessorMachine : public AResourceProcessor
{
	GENERATED_BODY()

public:
	AOreProcessorMachine();

	virtual void Tick(float DeltaSeconds) override;
	virtual bool TryStartProcessing() override;
	virtual void CancelProcessing() override;
	virtual bool CanAcceptItem_Implementation(const FItemStack& Item) const override;
	virtual bool AcceptItem_Implementation(const FItemStack& Item) override;

	/**
	 * AI parking/delivery transform authored by the Blueprint InputPoint component.
	 * This point belongs on reachable ground outside the receiving bin; it is not
	 * the visual spawn position of the deposited ore.
	 */
	UFUNCTION(BlueprintPure, Category="Mining|Ore Processor")
	FTransform GetDeliveryPointWorldTransform() const;

	UFUNCTION(BlueprintPure, Category="Mining|Ore Processor")
	int32 GetQueuedOreCount() const { return QueuedOreCount; }

	UFUNCTION(BlueprintPure, Category="Mining|Ore Processor")
	int32 GetProcessingQueueCapacity() const { return ProcessingQueueCapacity; }

	UFUNCTION(BlueprintPure, Category="Mining|Ore Processor")
	bool IsProcessingQueueFull() const { return QueuedOreCount >= ProcessingQueueCapacity; }

	UFUNCTION(BlueprintPure, Category="Mining|Ore Processor")
	EOreProcessorMachineState GetMachineState() const { return MachineState; }

	UPROPERTY(BlueprintAssignable, Category="Mining|Ore Processor")
	FOnProcessorQueueChanged OnProcessorQueueChanged;

	UPROPERTY(BlueprintAssignable, Category="Mining|Ore Processor")
	FOnOreProcessorStateChanged OnMachineStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Queue", meta=(ClampMin="1", ClampMax="1"))
	int32 ProcessingQueueCapacity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore Processor|Queue", meta=(ClampMin="1"))
	int32 MaxBufferedInputOre = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore Processor|Transport", meta=(ClampMin="1.0"))
	float InputOreTravelSpeed = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore Processor|Transport", meta=(ClampMin="1.0"))
	float OutputCoinTravelSpeed = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore Processor|Transport", meta=(ClampMin="0.0"))
	float WaitingOreSpacing = 12.0f;

	/** Small visual cadence so separately delivered ores do not overlap on the belt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Ore Processor|Transport", meta=(ClampMin="0.0", ClampMax="2.0"))
	float InputReleaseInterval = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Transport")
	FName InputSplineComponentName = TEXT("OreFlowSpline_Input");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Transport")
	FName OutputSplineComponentName = TEXT("OreFlowSpline_Output");

	/** Reachable parking point used by OreBuddy before it turns and unloads. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Transport")
	FName InputPointComponentName = TEXT("InputPoint");

	/** Authored point where a finished coin becomes a physical, collectable pickup. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Transport")
	FName OutputPointComponentName = TEXT("OutputPoint");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Pickup")
	TSubclassOf<AItemPickup> InputOrePickupClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Pickup")
	TSubclassOf<AItemPickup> CoinPickupClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Pickup")
	TObjectPtr<UStaticMesh> InputOreMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Pickup")
	TObjectPtr<UStaticMesh> CoinMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Pickup", meta=(ClampMin="0.01"))
	float InputOreMeshScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Pickup", meta=(ClampMin="0.01"))
	float CoinMeshScale = MineLearningItemVisual::GoldCoinScale;

	/** Legacy field retained for serialized compatibility; navigation now uses authored box obstacles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mining|Ore Processor|Navigation")
	FName NavigationBodyMeshComponentName = TEXT("StaticMesh5");

private:
	USplineComponent* FindAuthoredSpline(FName ComponentName) const;
	USceneComponent* FindAuthoredSceneComponent(FName ComponentName) const;
	AItemPickup* SpawnTransportPickup(const FItemStack& Stack, UStaticMesh* Mesh, float MeshScale, TSubclassOf<AItemPickup> PickupClass, const FTransform& SpawnTransform);
	void UpdateInputTransport(float DeltaSeconds);
	void UpdateOutputTransport(float DeltaSeconds);
	void TryAdmitWaitingOre();
	void AdmitOreAtProcessInput(int32 TransportIndex);
	void CompleteMachineProcessing();
	bool SpawnOutputCoin();
	void RefreshMachineState();
	void RefreshTickEnabled();
	void UpdateMachineVisualState();
	void UpdateRollerMotion(float DeltaSeconds);
	int32 GetBufferedOreCount() const;
	FVector GetWaitingOreLocation(int32 WaitingIndex) const;

	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> InputSpline = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> OutputSpline = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> CachedInputPoint = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> CachedOutputPoint = nullptr;

	UPROPERTY(Transient)
	TArray<FOreProcessorTransportItem> InputTransportItems;

	UPROPERTY(Transient)
	TArray<FOreProcessorTransportItem> OutputTransportItems;

	UPROPERTY(VisibleAnywhere, Category="Mining|Ore Processor|Queue")
	int32 QueuedOreCount = 0;

	UPROPERTY(VisibleAnywhere, Category="Mining|Ore Processor|State")
	EOreProcessorMachineState MachineState = EOreProcessorMachineState::Idle;

	FTimerHandle MachineProcessingTimerHandle;
	float NextInputReleaseTimeSeconds = 0.0f;
};
