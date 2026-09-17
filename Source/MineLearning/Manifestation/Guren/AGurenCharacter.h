#pragma once

#include "CoreMinimal.h"
#include "MineLearning/MineLearningCharacter.h"
#include "AGurenCharacter.generated.h"


class UInputAction;
class UGurenQSkillComponent;
class UMotionWarpingComponent;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGurenFlightStateChanged, float, Energy, float, Maximum, bool, bFlying);

UCLASS()
class MINELEARNING_API AGurenCharacter : public AMineLearningCharacter
{
	GENERATED_BODY()

public:
	AGurenCharacter();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Q Skill")
	TObjectPtr<UGurenQSkillComponent> QSkill;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Q Skill")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;
	virtual void Jump() override;
	virtual void StopJumping() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void UnPossessed() override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	UFUNCTION(BlueprintPure, Category = "Flight")
	bool IsFlightActive() const;

	UFUNCTION(BlueprintPure, Category = "Flight")
	float GetFlightEnergy() const { return FlightEnergy; }

	UFUNCTION(BlueprintPure, Category = "Flight")
	float GetMaxFlightEnergy() const { return MaxFlightEnergy; }

	UPROPERTY(BlueprintAssignable, Category = "Flight")
	FGurenFlightStateChanged OnFlightStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> BoostAction;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float NormalFlySpeed = 1800.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float BoostFlySpeed = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flight|Input")
	TObjectPtr<UInputMappingContext> FlightMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Flight|Input")
	TObjectPtr<UInputAction> FlightVerticalAction;

	UPROPERTY(EditDefaultsOnly, Category = "Flight", meta = (ClampMin = "1"))
	float MaxFlightEnergy = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flight", meta = (ClampMin = "0"))
	float FlightDrainPerSecond = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flight", meta = (ClampMin = "0"))
	float FlightRegenPerSecond = 10.f;

	float FlightEnergy = 100.f;
	float AirborneHoldTime = 0.f;
	float FlightGraceRemaining = 0.f;
	bool bFlightHeld = false;
	bool bFlightExhausted = false;

	void StartBoost();
	void StartQSkill();
	void StopBoost();
};
