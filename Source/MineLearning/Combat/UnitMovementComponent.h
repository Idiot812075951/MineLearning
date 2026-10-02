#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitMovementComponent.generated.h"

class UCombatComponent;
class UCharacterMovementComponent;

/** The one writer of effective walking speed; sprint supplies only its own multiplier. */
UCLASS()
class MINELEARNING_API UUnitMovementComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUnitMovementComponent();
	void SetLocomotionScale(float Scale);
	float GetBaseWalkSpeed() const { return BaseWalkSpeed; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UFUNCTION() void UpdateSpeed();
	UPROPERTY(Transient) TObjectPtr<UCombatComponent> Combat;
	UPROPERTY(Transient) TObjectPtr<UCharacterMovementComponent> Movement;
	float BaseWalkSpeed = 0.f;
	float LocomotionScale = 1.f;
};
