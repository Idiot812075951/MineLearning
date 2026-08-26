#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MiningToolComponent.generated.h"

class AMineableOre;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiningFinishedSignature, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMiningHitConfirmedSignature, FVector, HitLocation, FVector, HitNormal);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UMiningToolComponent : public UActorComponent
{
	GENERATED_BODY()


public:
	UMiningToolComponent();

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool StartMiningTarget(AMineableOre* TargetOre);

	UFUNCTION(BlueprintPure, Category="Mining")
	bool IsMining() const;

	/** Selects the mesh socket used as the mining contact origin. */
	UFUNCTION(BlueprintCallable, Category="Mining|Tool")
	void SetMiningHitSocketName(FName NewSocketName);

	void CancelMining();

	UPROPERTY(BlueprintAssignable, Category="Mining")
	FOnMiningFinishedSignature OnMiningFinished;

	/** Fired exactly once after a targeted mining hit has been accepted by the ore. */
	UPROPERTY(BlueprintAssignable, Category="Mining|Feedback")
	FOnMiningHitConfirmedSignature OnMiningHitConfirmed;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Animation")
	UAnimMontage* MiningMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Animation")
	bool bLockMovementDuringMining = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	bool bIsMining = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float MiningPower = 20.0f;

	/** Confirmed hits distributed evenly through one Loop segment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool", meta=(ClampMin="1", ClampMax="5", UIMin="1", UIMax="5"))
	int32 MiningHitCount = 5;

	/** Play rate for the complete Start -> Loop -> End montage. Capped so anticipation and recovery remain readable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Animation", meta=(ClampMin="0.1", ClampMax="2.0", UIMin="0.1", UIMax="2.0"))
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	FName HitSocketName = TEXT("PickaxeRightSocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	bool bUseOwnerMeshSocket = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	float StartForwardOffset = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	float StartHeightOffset = 60.0f;

private:
	static constexpr int32 MinMiningHitCount = 1;
	static constexpr int32 MaxMiningHitCount = 5;
	static constexpr float MinAttackSpeed = 0.1f;
	static constexpr float MaxAttackSpeed = 2.0f;

	UPROPERTY()
	AMineableOre* ActiveMiningTarget = nullptr;

	FTimerHandle MiningHitTimerHandle;

	int32 ActiveMiningHitCount = 0;
	int32 NextMiningHitIndex = 0;
	float MiningLoopStartTime = 0.0f;
	float MiningLoopEndTime = 0.0f;
	bool bMovementAndRotationLocked = false;
	bool bHadMovementComponent = false;
	EMovementMode PreviousMovementMode = MOVE_Walking;
	uint8 PreviousCustomMovementMode = 0;
	bool bPreviousOrientRotationToMovement = false;
	bool bPreviousUseControllerDesiredRotation = false;
	bool bPreviousUseControllerRotationYaw = false;

private:
	void FinishMining(bool bInterrupted, bool bBroadcastCompletion);
	bool PlayMiningMontage();
	bool ResolveMiningLoopRange();
	void ScheduleNextMiningHit();
	void HandleScheduledMiningHit();
	void StopScheduledMiningHits();
	int32 GetClampedMiningHitCount() const;
	float GetClampedAttackSpeed() const;
	float GetMiningHitMontageTime(int32 HitIndex) const;
	bool ApplyMiningHitToTarget(AMineableOre* TargetOre);
	void LockOwnerMovementAndRotation();
	void RestoreOwnerMovementAndRotation();

	void OnMiningMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	ACharacter* GetOwnerCharacter() const;
	USkeletalMeshComponent* GetOwnerMesh() const;
	FVector GetMiningHitCenter() const;
};
