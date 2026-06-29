#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "Navigation/PathFollowingComponent.h"
#include "MiningCompanionAIController.generated.h"

class AMineableOre;
class AMiningCompanionCharacter;
class AResourceDepot;
class AResourcePickup;
class UResourceCarryComponent;

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

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	UPROPERTY()
	AMiningCompanionCharacter* Companion = nullptr;

	UPROPERTY()
	AMineableOre* TargetOre = nullptr;

	UPROPERTY()
	AResourcePickup* TargetPickup = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Mining AI")
	EMiningCompanionState State = EMiningCompanionState::Idle;

	UPROPERTY(EditAnywhere, Category = "Mining AI")
	float SearchRadius = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI")
	float MiningInteractRadius = 135.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI")
	float MiningInterval = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Resource")
	float PickupSearchRadius = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Resource")
	float PickupInteractRadius = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Delivery")
	float DeliveryAcceptanceRadius = 120.0f;

	UPROPERTY(EditInstanceOnly, Category = "Mining AI|Delivery")
	AResourceDepot* DeliveryDepot = nullptr;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation")
	UAnimMontage* CollectMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation")
	FName CollectNotifyName = TEXT("Collect");

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation")
	UAnimMontage* DepositMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Mining AI|Animation")
	FName DepositNotifyName = TEXT("Deposit");

	float LastMiningTime = -999.0f;

private:
	void CacheCompanion();
	bool FindPickup();
	void FindOre();
	void RequestMoveToPickup();
	void RequestMoveToOre();
	void UpdateMoveToPickup(float DeltaSeconds);
	void UpdateMoveToOre(float DeltaSeconds);
	void UpdateMining(float DeltaSeconds);
	void UpdateReturningToDelivery(float DeltaSeconds);

	bool IsTargetOreValid() const;
	bool IsTargetPickupValid() const;
	bool IsCarryFull() const;
	UResourceCarryComponent* GetCarryComponent() const;

	void EnterMiningState();
	void RequestReturnToDelivery();
	void DepositCarriedOre();
	void FindDeliveryDepot();
	bool TryCollectTargetPickup();
	void StartCollectAction();
	void StartDepositAction();
	bool PlayActionMontage(UAnimMontage* Montage);

	UFUNCTION()
	void OnActionMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

	void OnActionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void ResetToIdle();
	void FaceTargetOre();
	void StopCompanionMovement();
};
