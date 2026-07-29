#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MiningTypes.h"
#include "MiningToolComponent.generated.h"

class AMineableOre;
class UAnimInstance;
class USkeletalMeshComponent;

struct FBranchingPointNotifyPayload;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiningFinishedSignature, bool, bInterrupted);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UMiningToolComponent : public UActorComponent
{
	GENERATED_BODY()


public:
	UMiningToolComponent();

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool StartMining();

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool StartMiningTarget(AMineableOre* TargetOre);

	UFUNCTION(BlueprintCallable, Category="Mining")
	void HandleMiningHitNotify();

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool TryMine();

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool TryMineTarget(AMineableOre* TargetOre);

	UFUNCTION(BlueprintPure, Category="Mining")
	bool IsMining() const;

	void CancelMining();

	UPROPERTY(BlueprintAssignable, Category="Mining")
	FOnMiningFinishedSignature OnMiningFinished;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Animation")
	UAnimMontage* MiningMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Animation")
	FName MiningHitNotifyName = TEXT("Mine_End");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Animation")
	bool bLockMovementDuringMining = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	bool bIsMining = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float MiningPower = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float TraceRadius = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float AttackInterval = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	FName HitSocketName = TEXT("PickaxeRightSocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	bool bUseOwnerMeshSocket = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	float StartForwardOffset = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	float StartHeightOffset = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Animation")
	bool bUseMiningMontage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Animation", meta=(EditCondition="!bUseMiningMontage"))
	float NonMontageMiningDuration = 0.25f;
private:
	UPROPERTY()
	AMineableOre* ActiveMiningTarget = nullptr;

	double LastMineTime = -999.0;

	FTimerHandle EndMiningTimerHandle;

	bool bStartedWithTarget = false;
	bool bMovementAndRotationLocked = false;
	bool bHadMovementComponent = false;
	EMovementMode PreviousMovementMode = MOVE_Walking;
	uint8 PreviousCustomMovementMode = 0;
	bool bPreviousOrientRotationToMovement = false;
	bool bPreviousUseControllerDesiredRotation = false;
	bool bPreviousUseControllerRotationYaw = false;

private:
	void EndMining();
	void FinishMining(bool bInterrupted, bool bBroadcastCompletion);
	bool PlayMiningMontage();
	bool ApplyMiningHitToTarget(AMineableOre* TargetOre);
	void LockOwnerMovementAndRotation();
	void RestoreOwnerMovementAndRotation();

	UFUNCTION()
	void OnMiningMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

	void OnMiningMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	ACharacter* GetOwnerCharacter() const;
	USkeletalMeshComponent* GetOwnerMesh() const;
	FVector GetMiningHitCenter() const;
};
