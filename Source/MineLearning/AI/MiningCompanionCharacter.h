#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "MineLearning/TransformationGuard.h"
#include "AutonomousUnit.h"
#include "MiningCompanionCharacter.generated.h"

class AMiningCompanionAIController;
class AMineableOre;
class AItemPickup;
class APlayerController;
class UMiningToolComponent;
class UNiagaraSystem;
class UResourceCarryComponent;
class USoundBase;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FMiningCompanionSkillAvailabilityChangedSignature,
	bool, bMiningAvailable,
	bool, bPickupAvailable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FMiningCompanionControlModeChangedSignature,
	bool, bPlayerControlled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOreBuddySprintChanged, float, StaminaFraction, bool, bSprinting);

UCLASS(BlueprintType)
class MINELEARNING_API AMiningCompanionCharacter : public ACharacter, public ITransformationGuard, public IAutonomousUnit
{
	GENERATED_BODY()

public:
	AMiningCompanionCharacter();
	virtual bool SupportsAutonomousControl() const override { return AIControllerClass != nullptr; }
	virtual bool CanTransform() const override { return !IsPlayerActionLocked() && !IsSprinting(); }
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category="Mining")
	UMiningToolComponent* GetMiningToolComponent() const { return MiningToolComponent; }

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	UResourceCarryComponent* GetResourceCarryComponent() const { return ResourceCarryComponent; }

	UFUNCTION(BlueprintPure, Category="Player Control|Skills")
	bool IsMiningSkillAvailable() const { return bMiningSkillAvailable; }

	UFUNCTION(BlueprintPure, Category="Player Control|Skills")
	bool IsPickupSkillAvailable() const { return bPickupSkillAvailable; }

	UPROPERTY(BlueprintAssignable, Category="Player Control|Skills")
	FMiningCompanionSkillAvailabilityChangedSignature OnSkillAvailabilityChanged;

	UPROPERTY(BlueprintAssignable, Category="Player Control")
	FMiningCompanionControlModeChangedSignature OnControlModeChanged;
	void TryUseMiningSkill();
	void TryUsePickupSkill();
	bool TryDeliverToNearbyMachine();
	UFUNCTION(BlueprintPure, Category="Player Control|Sprint")
	float GetSprintStamina() const { return SprintStamina; }
	UFUNCTION(BlueprintPure, Category="Player Control|Sprint")
	bool IsSprinting() const { return bSprinting; }
	UPROPERTY(BlueprintAssignable, Category="Player Control|Sprint")
	FOreBuddySprintChanged OnSprintChanged;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Player Control|Sprint", meta=(ClampMin="1.0"))
	float SprintSpeedMultiplier = 1.8f;
	UPROPERTY(EditDefaultsOnly, Category="Player Control|Sprint", meta=(ClampMin="0.1"))
	float SprintDuration = 3.f;
	UPROPERTY(EditDefaultsOnly, Category="Player Control|Sprint", meta=(ClampMin="0.1"))
	float SprintRecoveryDuration = 4.f;
	UPROPERTY(EditDefaultsOnly, Category="Player Control|Sprint", meta=(ClampMin="0.0"))
	float SprintRecoveryDelay = 0.8f;
	UPROPERTY(EditDefaultsOnly, Category="Player Control|Sprint", meta=(ClampMin="0.01"))
	float SprintAccelerationTime = 0.35f;
	UPROPERTY(EditDefaultsOnly, Category="Player Control|Sprint", meta=(ClampMin="0.01"))
	float SprintDecelerationTime = 0.45f;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Player Control")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Player Control")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY()
	TObjectPtr<UInputAction> PickupSkillAction;

	UPROPERTY(EditDefaultsOnly, Category="Player Control|Mining", meta=(ClampMin="0.0"))
	float PlayerMiningInteractRadius = 135.0f;

	UPROPERTY(EditDefaultsOnly, Category="Player Control|Pickup", meta=(ClampMin="0.0"))
	float PlayerPickupInteractRadius = 135.0f;

	/** Horizontal radius around the receiver's authored delivery point. */
	UPROPERTY(EditDefaultsOnly, Category="Player Control|Delivery", meta=(ClampMin="0.0"))
	float AutoDeliveryRadius = 120.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	UMiningToolComponent* MiningToolComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Carry")
	UResourceCarryComponent* ResourceCarryComponent;

	/** OreBuddy-only confirmed-hit sparks. Gameplay confirmation remains in MiningToolComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Feedback")
	TObjectPtr<UNiagaraSystem> MiningImpactSystem;

	/** Optional impact sound hook. Empty until a real project sound asset is supplied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Feedback")
	TObjectPtr<USoundBase> MiningImpactSound;

private:
	float SprintMovementScale = 1.f;
	void UpdateSprint(float DeltaSeconds);
	float SprintStamina = 1.f;
	float NormalWalkSpeed = 0.f;
	float SprintRecoveryRemaining = 0.f;
	bool bSprinting = false;
	bool bSprintExhausted = false;
	enum class EPlayerInteractionState : uint8
	{
		None,
		AligningCollect,
		Collecting,
		AligningDeposit,
		Depositing
	};

	AMineableOre* FindMineableOreInRange() const;
	AItemPickup* FindPickupInRange();
	float GetSquaredDistanceToOre(const AMineableOre* Ore, FVector* OutClosestPoint = nullptr) const;
	void StartPlayerInteractionMonitoring(APlayerController* PlayerController);
	void RefreshPlayerInteractionState();
	void TryAutoDeposit();
	bool StartPlayerCollectAction(AItemPickup* Pickup);
	bool StartPlayerDepositAction(AActor* Receiver);
	void UpdatePlayerInteractionAlignment(float DeltaSeconds);
	bool PlayPlayerActionMontage(UAnimMontage* Montage, float PlayRate = 1.0f);
	void FinishPlayerInteraction();
	void LockPlayerInteraction();
	void UnlockPlayerInteraction();
	bool IsPlayerActionLocked() const;
	const AMiningCompanionAIController* GetMiningAIConfig() const;
	void CommitPlayerPickup();
	void CommitPlayerDeposit();
	FTransform GetDeliveryPointTransform(AActor* Receiver) const;
	void ShowNoMineableOreMessage() const;
	void ShowNoPickupMessage() const;
	void ShowCarryFullMessage() const;

	UFUNCTION()
	void OnPlayerActionMontageNotifyBegin(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload);

	void OnPlayerActionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleMiningHitConfirmed(FVector HitLocation, FVector HitNormal);

	UPROPERTY(Transient)
	TObjectPtr<AItemPickup> PlayerInteractionPickup;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PlayerInteractionReceiver;

	FTimerHandle PlayerInteractionTimerHandle;
	EPlayerInteractionState PlayerInteractionState = EPlayerInteractionState::None;
	TEnumAsByte<EMovementMode> PreviousPlayerMovementMode = MOVE_Walking;
	uint8 PreviousPlayerCustomMovementMode = 0;
	bool bPreviousPlayerOrientRotationToMovement = true;
	bool bPreviousPlayerUseControllerDesiredRotation = false;
	bool bPlayerInteractionMovementLocked = false;
	bool bMiningSkillAvailable = false;
	bool bPickupSkillAvailable = false;
};
