#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OreVisualComponent.generated.h"

class AMineableOre;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/** A mesh to display while the ore's remaining-health ratio is at or above MinHealthRatio. */
USTRUCT(BlueprintType)
struct FOreVisualStage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Visual", meta=(ClampMin="0.0", ClampMax="1.0"))
	float MinHealthRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Visual")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	/** Optional material override for meshes whose imported material is missing or unsuitable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Visual")
	TObjectPtr<UMaterialInterface> MaterialOverride = nullptr;
};

/**
 * Presentation-only stage selector for AMineableOre.
 * It observes health changes and swaps the owner's existing OreMesh; it owns no mining or drop state.
 */
UCLASS(ClassGroup=(Mining), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UOreVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOreVisualComponent();

	/** Re-evaluates the stage from the ore's current health. Useful after runtime configuration changes. */
	UFUNCTION(BlueprintCallable, Category="Mining|Visual")
	void RefreshVisualStage();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual")
	TArray<FOreVisualStage> Stages;

	/** Amount to contract from the authored OreMesh relative scale on a normal mining hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Hit Punch", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PunchStrength = 0.035f;

	/** Amount to overshoot above the authored OreMesh relative scale after the contraction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Hit Punch", meta=(ClampMin="0.0"))
	float PunchOvershoot = 0.015f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Hit Punch", meta=(ClampMin="0.0"))
	float PunchToOvershootDelay = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Hit Punch", meta=(ClampMin="0.0"))
	float PunchRecoveryDelay = 0.055f;

	/** Stronger contraction used only when damage switches to a different configured visual stage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Hit Punch", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StageBreakPunchStrength = 0.12f;

	/** Stronger overshoot used only when damage switches to a different configured visual stage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Hit Punch", meta=(ClampMin="0.0"))
	float StageBreakPunchOvershoot = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Hit Punch", meta=(ClampMin="0.0"))
	float StageBreakPunchToOvershootDelay = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Hit Punch", meta=(ClampMin="0.0"))
	float StageBreakPunchRecoveryDelay = 0.08f;

	/** Purely visual meshes spawned when a mining hit switches to another visual stage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Stage Debris")
	TArray<TObjectPtr<UStaticMesh>> VisualDebrisMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Stage Debris", meta=(ClampMin="0"))
	int32 DebrisCountMin = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Stage Debris", meta=(ClampMin="0"))
	int32 DebrisCountMax = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Stage Debris", meta=(ClampMin="0.0"))
	float DebrisScaleMin = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Stage Debris", meta=(ClampMin="0.0"))
	float DebrisScaleMax = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Stage Debris", meta=(ClampMin="0.0"))
	float DebrisLifetime = 1.5f;

	/** Velocity-change impulse magnitude; each debris piece launches randomly outward and upward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Stage Debris", meta=(ClampMin="0.0"))
	float DebrisImpulseMin = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Visual|Stage Debris", meta=(ClampMin="0.0"))
	float DebrisImpulseMax = 450.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleOreHealthChanged(float CurrentHealth, float MaxHealth);

	const FOreVisualStage* FindVisualStage(int32 MiningStageIndex) const;
	bool ApplyVisualStage(int32 MiningStageIndex);
	void PlayHitVisual();
	void PlayStageBreakVisual();
	void PlayPunch(float InStrength, float InOvershoot, float InToOvershootDelay, float InRecoveryDelay);
	void SpawnVisualDebris();
	void ApplyPunchOvershoot();
	void RestoreBaseScale();
	void ClearHitPunchTimers();

	TWeakObjectPtr<AMineableOre> OwningOre;
	TWeakObjectPtr<UStaticMeshComponent> TargetMesh;
	FVector BaseRelativeScale = FVector::OneVector;
	float ActivePunchOvershoot = 0.0f;
	float LastObservedHealth = 0.0f;
	bool bHasObservedHealth = false;
	FTimerHandle PunchOvershootTimerHandle;
	FTimerHandle PunchRestoreTimerHandle;
};
