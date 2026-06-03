#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
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
	float MiningDistance = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Mining AI")
	float MiningInterval = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Mining AI")
	float MoveInputScale = 1.0f;

	float LastMiningTime = -999.0f;

private:
	void CacheCompanion();
	void FindOre();
	void UpdateMoveToOre(float DeltaSeconds);
	void UpdateMining(float DeltaSeconds);

	bool IsTargetOreValid() const;
	void FaceTargetOre();
	void StopCompanionMovement();
};