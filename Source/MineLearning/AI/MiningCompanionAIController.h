#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "MiningCompanionAIController.generated.h"

class AMineableOre;
class AMiningCompanionCharacter;
class AItemPickup;
class UResourceCarryComponent;
class UMiningCompanionTargetingComponent;

UENUM(BlueprintType)
enum class EMiningCompanionState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	MoveToOre	UMETA(DisplayName = "Move To Ore"),
	Mining		UMETA(DisplayName = "Mining"),
	MoveToPickup	UMETA(DisplayName = "Move To Pickup"),
	Collecting	UMETA(DisplayName = "Collecting"),
	Depositing	UMETA(DisplayName = "Depositing"),
	ReturningToDelivery	UMETA(DisplayName = "Returning To Delivery")
};

UCLASS()
class MINELEARNING_API AMiningCompanionAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMiningCompanionAIController();

	UFUNCTION(BlueprintCallable, Category="Mining AI|Debug", meta=(DevelopmentOnly))
	void SetDebugPaused(bool bPaused);

	UFUNCTION(BlueprintPure, Category="Mining AI|Debug", meta=(DevelopmentOnly))
	bool IsDebugPaused() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mining AI|Debug", meta=(AllowPrivateAccess="true"))
	bool bDebugPaused = false;

	UPROPERTY()
	AMiningCompanionCharacter* Companion = nullptr;

	UPROPERTY()
	AMineableOre* TargetOre = nullptr;

	UPROPERTY()
	AItemPickup* TargetPickup = nullptr;

	UPROPERTY()
	AActor* DeliveryTarget = nullptr;

	UPROPERTY(VisibleAnywhere, Category="Mining AI|Targeting")
	TObjectPtr<UMiningCompanionTargetingComponent> TargetingComponent;

	UPROPERTY(VisibleAnywhere, Category = "Mining AI")
	EMiningCompanionState State = EMiningCompanionState::Idle;

	UPROPERTY(EditAnywhere, Category = "Mining AI")
	float SearchRadius = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI")
	float MiningInteractRadius = 135.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Resource")
	float PickupSearchRadius = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Resource")
	float PickupInteractRadius = 80.0f;

	/** Idle actors query the world at this interval rather than once per frame. */
	UPROPERTY(EditAnywhere, Category = "Mining AI|Search", meta=(ClampMin="0.05"))
	float IdleSearchInterval = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Delivery")
	float DeliveryAcceptanceRadius = 65.0f;

	/** Shared turn speed for pickup and delivery action alignment. */
	UPROPERTY(EditAnywhere, Category = "Mining AI|Delivery", meta=(ClampMin="1.0"))
	float DeliveryRotationSpeed = 180.0f;

	/** Shared yaw tolerance for pickup and delivery action alignment. */
	UPROPERTY(EditAnywhere, Category = "Mining AI|Delivery", meta=(ClampMin="0.1"))
	float DeliveryRotationTolerance = 1.0f;

	/** Short deterministic fallback used only when navigation accepts a request but the pawn remains stalled. */
	UPROPERTY(EditAnywhere, Category = "Mining AI|Navigation Fallback", meta=(ClampMin="1.0"))
	float DirectMoveSpeed = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Navigation Fallback", meta=(ClampMin="0.1"))
	float NavigationStallTimeout = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation")
	UAnimMontage* CollectMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation", meta=(ClampMin="0.1"))
	float CollectAnimationPlayRate = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation")
	FName CollectGrabNotifyName = TEXT("Notify_CollectGrab");

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation")
	FName CollectReleaseNotifyName = TEXT("Notify_CollectRelease");

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation")
	FName CollectSocketName = TEXT("S_ClawGrip");

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation")
	UAnimMontage* DepositMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation")
	FName DepositNotifyName = TEXT("Notify_DeliverResource");

private:
	void CacheCompanion();
	bool FindPickup();
	void FindOre();
	void RequestMoveToPickup();
	void RequestMoveToOre();
	void UpdateMoveToPickup(float DeltaSeconds);
	void UpdateMoveToOre(float DeltaSeconds);
	void UpdateReturningToDelivery(float DeltaSeconds);
	void TickDirectMove(float DeltaSeconds);
	void UpdateNavigationFallback(float DeltaSeconds);

	bool IsTargetOreValid() const;
	bool IsTargetPickupValid() const;
	bool IsCarryFull() const;
	UResourceCarryComponent* GetCarryComponent() const;

	void EnterMiningState();
	void RequestReturnToDelivery();
	void DepositCarriedItem();
	void FindDeliveryTarget();
	FTransform GetDeliveryPointTransform() const;
	FVector GetDeliveryNavigationLocation() const;
	void BeginPickupAlignment();
	void BeginDeliveryAlignment();
	bool RotateCompanionTowards(FRotator TargetRotation, float DeltaSeconds);
	bool TryCollectTargetPickup();
	void StartCollectAction();
	void StartDepositAction();
	bool PlayActionMontage(UAnimMontage* Montage, float PlayRate = 1.0f);
	void LockCollectMovementAndRotation();
	void RestoreCollectMovementAndRotation();
	void CancelTargetPickup();

	UFUNCTION()
	void OnActionMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

	void OnActionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnMiningFinished(bool bInterrupted);

	void ResetToIdle();
	void FaceTargetOre();
	void StopCompanionMovement();

	bool bCollectMovementLocked = false;
	bool bCollectPreviousUseControllerRotationYaw = false;
	bool bCollectPreviousOrientRotationToMovement = false;
	bool bCollectPreviousUseControllerDesiredRotation = false;
	TEnumAsByte<EMovementMode> CollectPreviousMovementMode = MOVE_Walking;
	uint8 CollectPreviousCustomMovementMode = 0;
	bool bAligningForAction = false;
	bool bDirectMove = false;
	float NavigationStallSeconds = 0.0f;
	float NextIdleSearchTime = 0.0f;
};
