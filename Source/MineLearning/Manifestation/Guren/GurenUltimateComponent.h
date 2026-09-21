#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GurenUltimateComponent.generated.h"

class UGrabbableComponent;
class APlayerController;
class UBrainComponent;

UENUM(BlueprintType)
enum class EGurenUltimateStage : uint8 { Idle, Ready, Launch, Impact, Descent, Arrival, Burst, Recover };

struct FArrivalTarget
{
	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<UGrabbableComponent> Reservation;
	FVector Location = FVector::ZeroVector;
	float Radius = 100.f;
	bool bDamageEnabled = true;
	bool bPierced = false;
	bool bExecuted = false;
	bool bExecuteEligible = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGurenUltimateStageChanged, EGurenUltimateStage, Stage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGurenUltimateTargetPierced, int32, TargetIndex);

/** Owns the timeline, reservations and delayed settlement. Presentation observes these facts. */
UCLASS(ClassGroup = (Guren), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UGurenUltimateComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGurenUltimateComponent();
	UFUNCTION(BlueprintCallable, Category = "Arrival") bool TryStart(AActor* RequestedTarget = nullptr);
	UFUNCTION(BlueprintCallable, Category = "Arrival") void HandleBeat(EGurenUltimateStage Beat);
	UFUNCTION(BlueprintCallable, Category = "Arrival") void Abort();
	UFUNCTION(BlueprintPure, Category = "Arrival") bool IsUltimateActive() const { return Stage != EGurenUltimateStage::Idle; }
	UFUNCTION(BlueprintPure, Category = "Arrival") EGurenUltimateStage GetStage() const { return Stage; }
	UFUNCTION(BlueprintPure, Category = "Arrival") AActor* GetTarget() const { return Targets.IsEmpty() ? nullptr : Targets[0].Actor.Get(); }
	UFUNCTION(BlueprintPure, Category = "Arrival") int32 GetTargetCount() const { return Targets.Num(); }
	UFUNCTION(BlueprintPure, Category = "Arrival") FVector GetHitLocation() const { return Targets.IsEmpty() ? FVector::ZeroVector : Targets[0].Location; }
	UFUNCTION(BlueprintPure, Category = "Arrival") float GetTargetRadius() const { return Targets.IsEmpty() ? 100.f : Targets[0].Radius; }
	UFUNCTION(BlueprintPure, Category = "Arrival") FVector GetArrivalLocation() const { return ArrivalLocation; }
	UFUNCTION(BlueprintPure, Category = "Arrival") FVector GetLaunchLocation() const { return LaunchLocation; }
	UFUNCTION(BlueprintPure, Category = "Arrival") FVector GetAttackDirection() const { return AttackDirection; }
	UFUNCTION(BlueprintPure, Category = "Arrival") FVector GetFlightLocation() const;
	UFUNCTION(BlueprintPure, Category = "Arrival") bool HasGround() const { return bHasGround; }
	UFUNCTION(BlueprintPure, Category = "Arrival") float GetStageTime() const { return StageTime; }
	UFUNCTION(BlueprintPure, Category = "Arrival") float GetStageDuration(EGurenUltimateStage InStage) const;
	const TArray<FArrivalTarget>& GetTargets() const { return Targets; }
	FBox GetTargetBounds() const;
	static FVector EvaluateArc(const FVector& Start, const FVector& End, const FVector& Side, float Width, float Alpha);
	UPROPERTY(BlueprintAssignable, Category = "Arrival") FGurenUltimateStageChanged OnStageChanged;
	UPROPERTY(BlueprintAssignable, Category = "Arrival") FGurenUltimateTargetPierced OnTargetPierced;
	UPROPERTY(EditAnywhere, Category = "Arrival|Targets", meta = (ClampMin = "1", Units = "cm")) float Range = 3200.f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Targets", meta = (ClampMin = "1", ClampMax = "12")) int32 MaxTargets = 6;
	UPROPERTY(EditAnywhere, Category = "Arrival|Targets", meta = (ClampMin = "1", Units = "cm")) float ChainRange = 2600.f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Targets") FName HitSocket = TEXT("RadiantHitSocket");
	/** Clearance from the ground to the feet, shared by ground and airborne starts. */
	UPROPERTY(EditAnywhere, Category = "Arrival|Flight", meta = (ClampMin = "100", Units = "cm")) float LaunchHeight = 1200.f;
	/** Ground clearance during the final wing pose and settlement. */
	UPROPERTY(EditAnywhere, Category = "Arrival|Flight", meta = (ClampMin = "0", Units = "cm")) float FinalHoverHeight = 1200.f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Flight", meta = (ClampMin = "0", Units = "cm")) float ArcWidth = 150.f;
	/** Scales outbound and return speed only; pose and settlement timing remain unchanged. */
	UPROPERTY(EditAnywhere, Category = "Arrival|Flight", meta = (ClampMin = "0.1", UIMax = "5")) float FlightSpeedMultiplier = 1.5f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Flight") FName LaunchSocket = TEXT("radiant_forearm_r");
	/** Ready, outbound weave, return, descent, hero hold, settlement, recovery. */
	UPROPERTY(EditAnywhere, Category = "Arrival|Timing", meta = (EditFixedSize, ClampMin = "0.1")) TArray<float> StageDurations = { 1.35f, 2.8f, 0.55f, 1.3f, 0.9f, 0.65f, 0.45f };
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	struct FHold
	{
		TWeakObjectPtr<AActor> Actor;
		TWeakObjectPtr<UBrainComponent> Brain;
		bool bTick = false;
		float TimeDilation = 1.f;
	};
	EGurenUltimateStage Stage = EGurenUltimateStage::Idle;
	TArray<FArrivalTarget> Targets;
	TWeakObjectPtr<APlayerController> Player;
	TArray<FHold> Holds;
	FTransform OriginalTransform;
	FVector LaunchLocation = FVector::ZeroVector;
	FVector FlightOrigin = FVector::ZeroVector;
	FVector ArrivalLocation = FVector::ZeroVector;
	FVector AttackDirection = FVector::ForwardVector;
	FVector OriginalVelocity = FVector::ZeroVector;
	float StageTime = 0.f;
	EMovementMode OriginalMovementMode = MOVE_Walking;
	uint8 OriginalCustomMode = 0;
	bool bHasGround = false;
	bool bOriginalDamage = true;
	bool bOriginalInputEnabled = true;
	bool bOriginalOrient = true;
	bool bCommitted = false;
	bool bExiting = false;
	bool IsValidTarget(AActor* Candidate) const;
	void AddTarget(AActor* Actor);
	void SetStage(EGurenUltimateStage NewStage);
	void UpdatePhase();
	void Exit(bool bCancelled);
};
