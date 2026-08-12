#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GunnerAIController.generated.h"

class AGunnerCharacter;
class AMineableOre;

UENUM(BlueprintType)
enum class EGunnerAIState : uint8
{
	Idle,
	MoveToOre,
	Attacking
};

UCLASS()
class MINELEARNING_API AGunnerAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGunnerAIController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	void CacheGunner();
	void FindOre();
	void RequestMoveToOre();
	void BeginAttacking();
	void UpdateAttacking(float DeltaSeconds);
	void ResetToIdle();
	bool IsTargetOreValid() const;
	void FaceTarget(float DeltaSeconds);

	UPROPERTY()
	TObjectPtr<AGunnerCharacter> Gunner;

	UPROPERTY()
	TObjectPtr<AMineableOre> TargetOre;

	UPROPERTY(VisibleInstanceOnly, Category="Gunner AI")
	EGunnerAIState State = EGunnerAIState::Idle;

	UPROPERTY(EditAnywhere, Category="Gunner AI", meta=(ClampMin="0.0"))
	float SearchRadius = 5000.0f;

	UPROPERTY(EditAnywhere, Category="Gunner AI", meta=(ClampMin="100.0"))
	float AttackRange = 900.0f;

	UPROPERTY(EditAnywhere, Category="Gunner AI", meta=(ClampMin="1.0"))
	float RotationSpeed = 360.0f;
};
