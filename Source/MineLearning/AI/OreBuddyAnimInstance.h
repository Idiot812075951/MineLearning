#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "OreBuddyAnimInstance.generated.h"

/**
 * OreBuddy-specific presentation layer applied after the Animation Blueprint pose.
 * Gameplay only retriggers a short impact pulse; this class owns the drill-only recoil.
 */
UCLASS(Transient, Blueprintable)
class MINELEARNING_API UOreBuddyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;

	/** Retriggers the drill recoil from full strength. Safe to call while recovering. */
	UFUNCTION(BlueprintCallable, Category="OreBuddy|Mining")
	void TriggerMiningImpact();

	UPROPERTY(BlueprintReadOnly, Transient, Category="OreBuddy|Mining")
	float MiningImpactAlpha = 0.0f;

protected:
	/** Physical recoil distance in Unreal centimeters. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OreBuddy|Mining", meta=(ClampMin="0.5", ClampMax="30", Units="cm"))
	float MiningRecoilDistanceCm =15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OreBuddy|Mining", meta=(ClampMin="0.08", ClampMax="3", Units="s"))
	float MiningRecoilDuration = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OreBuddy|Mining")
	FName DrillArmBoneName = TEXT("DrillArm");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OreBuddy|Mining")
	FName DrillBitBoneName = TEXT("DrillBit");

private:
	float MiningImpactElapsed = 0.0f;
};
