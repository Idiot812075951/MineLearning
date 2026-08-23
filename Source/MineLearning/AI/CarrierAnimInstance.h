#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CarrierAnimInstance.generated.h"

class AHaulerCharacter;

UCLASS(Transient, Blueprintable)
class MINELEARNING_API UCarrierAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category="Carrier|Locomotion")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Carrier|Locomotion")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category="Carrier|Cargo")
	bool bHasCargo = false;

private:
	UFUNCTION()
	void AnimNotify_Pickup();

	UFUNCTION()
	void AnimNotify_DropOff();

	UPROPERTY(Transient)
	TObjectPtr<AHaulerCharacter> CachedHauler;

	FVector PreviousOwnerLocation = FVector::ZeroVector;
	bool bHasOwnerLocationSample = false;
};
