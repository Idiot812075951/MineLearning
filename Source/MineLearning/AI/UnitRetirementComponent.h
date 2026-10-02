#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MineLearning/Combat/CombatTypes.h"
#include "UnitRetirementComponent.generated.h"

/** Stops an autonomous unit before destroying it. Presentation may observe the retirement event. */
UCLASS(ClassGroup=(Units), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UUnitRetirementComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUnitRetirementComponent();
	UFUNCTION(BlueprintCallable, Category="Units") void Retire();
	UFUNCTION(BlueprintPure, Category="Units") bool IsRetiring() const { return bRetiring; }
	UPROPERTY(EditAnywhere, Category="Units", meta=(ClampMin="0")) float RetirementDuration = 0.65f;
	UPROPERTY(BlueprintAssignable, Category="Units") FCombatStateChanged OnRetiring;
private:
	bool bRetiring = false;
};
