#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "HaulerAIController.generated.h"

class AHaulerCharacter;
class AItemPickup;
struct FItemStack;
class UResourceCarryComponent;
class UResourceStorageComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EHaulerState : uint8
{
	Idle,
	MovingToPickup,
	PickingUp,
	MovingToDestination,
	DroppingOff
};

UCLASS()
class MINELEARNING_API AHaulerAIController : public AAIController
{
	GENERATED_BODY()

public:
	AHaulerAIController();

	UFUNCTION(BlueprintPure, Category="Item|Hauler")
	EHaulerState GetHaulerState() const { return State; }

	UFUNCTION(BlueprintCallable, Category="Item|Hauler|Debug", meta=(DevelopmentOnly))
	void SearchNow();

	void HandlePickupAnimationNotify();
	void HandlePickupAnimationFinished();
	void HandleDropOffAnimationNotify();
	void HandleDropOffAnimationFinished();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	void CacheHauler();
	void TryFindWork();
	bool FindNearestValidPickup();
	bool MoveToCurrentPickup();
	bool BeginPickupAnimation();
	bool CollectCurrentPickup();
	bool ResolveAndMoveToDestination();
	bool BeginDropOffAnimation();
	bool DepositCurrentItem();
	void ResetToIdle();
	AActor* ResolvePickupDestination(const AItemPickup* Pickup) const;
	bool HasValidExplicitDeliveryRoute(const FItemStack& Item) const;
	FVector GetDestinationLocation() const;
	void TickDirectMove(float DeltaSeconds);

	UPROPERTY()
	TObjectPtr<AHaulerCharacter> Hauler;

	UPROPERTY()
	TObjectPtr<AItemPickup> TargetPickup;

	UPROPERTY()
	TObjectPtr<AActor> TargetDestination;

	UPROPERTY()
	TObjectPtr<UResourceStorageComponent> ExplicitDeliveryStorage;

	UPROPERTY()
	TObjectPtr<USceneComponent> ExplicitDeliveryPoint;

	UPROPERTY(VisibleAnywhere, Category="Item|Hauler")
	EHaulerState State = EHaulerState::Idle;

	UPROPERTY(EditAnywhere, Category="Item|Hauler", meta=(ClampMin="0.1"))
	float SearchInterval = 0.4f;

	UPROPERTY(EditAnywhere, Category="Item|Hauler")
	float PickupAcceptanceRadius = 90.0f;

	UPROPERTY(EditAnywhere, Category="Item|Hauler")
	float DestinationAcceptanceRadius = 160.0f;

	UPROPERTY(EditAnywhere, Category="Item|Hauler")
	float DirectMoveSpeed = 320.0f;

	/** Switch to the deterministic local fallback when a valid path makes no progress. */
	UPROPERTY(EditAnywhere, Category="Item|Hauler", meta=(ClampMin="0.1"))
	float NavigationStallTimeout = 1.0f;

	FTimerHandle SearchTimerHandle;
	bool bDirectMove = false;
	bool bIssuingMoveRequest = false;
	bool bPickupCommitted = false;
	bool bDropOffCommitted = false;
	float NavigationStallSeconds = 0.0f;
};
