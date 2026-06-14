#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "MiningCompanionAIController.generated.h"

class AMineableOre;
class AMiningCompanionCharacter;

UENUM(BlueprintType)
enum class EMiningCompanionState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	MoveToOre	UMETA(DisplayName = "Move To Ore"),
	Mining		UMETA(DisplayName = "Mining")
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

	UPROPERTY(VisibleAnywhere, Category = "Mining AI")
	EMiningCompanionState State = EMiningCompanionState::Idle;

	UPROPERTY(EditAnywhere, Category = "Mining AI")
	float SearchRadius = 3000.0f;
	
	UPROPERTY(EditAnywhere, Category = "Mining AI")
	float MiningStartDistance = 90.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI")
	float MiningInterval = 1.2f;

	float LastMiningTime = -999.0f;

private:
	//TODO 需要review代码，看看下面这么多东西哪些是没用的，而且现在挖矿寻路还有问题，会出现挖矿结束后不在寻找目标的问题
	void CacheCompanion();
	void FindOre();
	void RequestMoveToOre();
	void UpdateMoveToOre(float DeltaSeconds);
	void UpdateMining(float DeltaSeconds);

	bool IsTargetOreValid() const;
	bool IsCloseEnoughToMine() const;

	void EnterMiningState();
	void ResetToIdle();
	void FaceTargetOre();
	void StopCompanionMovement();
};