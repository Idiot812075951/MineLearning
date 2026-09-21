#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GurenQSkillComponent.generated.h"

class UGrabbableComponent;
class AController;

UENUM(BlueprintType)
enum class EGurenQStage : uint8
{
	Idle, Dash, Grab, Radiation, Release
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGurenQStageChanged, EGurenQStage, Stage, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGurenQContact);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGurenQTargetsChanged);

/** Notify-driven execution through the grabbable contract, independent of target classes and UI. */
UCLASS(ClassGroup = (Guren), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UGurenQSkillComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGurenQSkillComponent();
	UFUNCTION(BlueprintPure, Category = "Combat") bool CanExecuteTarget(AActor* Actor) const;
	UFUNCTION(BlueprintPure, Category = "Combat") float GetExecuteThreshold(float MaxHealth) const;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Execution", meta = (ClampMin = "0", ClampMax = "1")) float ExecuteHealthPercent = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Execution", meta = (ClampMin = "0")) float ExecuteHealthFlat = 1000.f;
	UFUNCTION(BlueprintCallable, Category = "Q Skill") void TryCast();
	UFUNCTION(BlueprintCallable, Category = "Q Skill") void CycleTarget();
	UFUNCTION(BlueprintCallable, Category = "Q Skill") void RefreshTargets();
	UFUNCTION(BlueprintCallable, Category = "Q Skill") void HandleAnimationEvent(FName Event);
	UFUNCTION(BlueprintCallable, Category = "Q Skill") void Cancel();
	UFUNCTION(BlueprintPure, Category = "Q Skill") bool IsQActive() const { return Stage != EGurenQStage::Idle; }
	UFUNCTION(BlueprintPure, Category = "Q Skill") EGurenQStage GetStage() const { return Stage; }
	UFUNCTION(BlueprintPure, Category = "Q Skill") AActor* GetTarget() const;
	UFUNCTION(BlueprintPure, Category = "Q Skill") UGrabbableComponent* GetSelectedTarget() const { return SelectedTarget.Get(); }
	UFUNCTION(BlueprintPure, Category = "Q Skill") TArray<UGrabbableComponent*> GetCandidates() const;
	UFUNCTION(BlueprintPure, Category = "Q Skill") float GetExecutionScale() const { return ExecutionScale; }
	UFUNCTION(BlueprintPure, Category = "Q Skill") FVector GetGripLocation() const;
	UPROPERTY(BlueprintAssignable, Category = "Q Skill") FGurenQStageChanged OnStageChanged;
	UPROPERTY(BlueprintAssignable, Category = "Q Skill") FGurenQContact OnGrabContact;
	UPROPERTY(BlueprintAssignable, Category = "Q Skill") FGurenQTargetsChanged OnTargetsChanged;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UPROPERTY(EditAnywhere, Category = "Q Skill|Selection", meta = (ClampMin = "1", Units = "cm")) float SelectionRange = 2000.f;
	UPROPERTY(EditAnywhere, Category = "Q Skill|Selection", meta = (ClampMin = "0.02", Units = "s")) float SelectionInterval = 0.1f;
	UPROPERTY(EditAnywhere, Category = "Q Skill|Selection") bool bRequireLineOfSight = true;
	UPROPERTY(EditAnywhere, Category = "Q Skill|Movement", meta = (ClampMin = "1", Units = "cm")) float DirectGrabRange = 200.f;
	UPROPERTY(EditAnywhere, Category = "Q Skill|Grip") FName GripSocket = TEXT("Q_GrabHead");
	/** Socket position at GrabContact, relative to the standing capsule center. */
	UPROPERTY(EditAnywhere, Category = "Q Skill|Grip") FVector ContactOffset = FVector(165.f, 53.f, 67.54f);
	UPROPERTY(EditAnywhere, Category = "Q Skill|Grip", meta = (ClampMin = "1", Units = "cm")) float ClawDiameter = 110.f;
	UPROPERTY(EditAnywhere, Category = "Q Skill|Grip", meta = (ClampMin = "1")) float MaximumScale = 12.f;
	UPROPERTY(EditAnywhere, Category = "Q Skill|Validation", meta = (ClampMin = "1", Units = "s")) float ExecutionTimeout = 8.f;
	UPROPERTY(EditAnywhere, Category = "Q Skill|Validation", meta = (ClampMin = "1", Units = "cm")) float ArrivalTolerance = 55.f;
	UPROPERTY(EditAnywhere, Category = "Q Skill|Validation", meta = (ClampMin = "1", Units = "cm")) float ContactTolerance = 150.f;
	UPROPERTY(EditAnywhere, Category = "Q Skill|Movement", meta = (ClampMin = "1")) float ReachSpeed = 25.f;
	EGurenQStage Stage = EGurenQStage::Idle;
	TArray<TWeakObjectPtr<UGrabbableComponent>> Candidates;
	TWeakObjectPtr<UGrabbableComponent> SelectedTarget;
	TWeakObjectPtr<UGrabbableComponent> Target;
	TWeakObjectPtr<AController> LockedController;
	FTransform OriginalMeshTransform;
	FTransform StandTransform;
	float ExecutionScale = 1.f;
	bool bOriginalOrientToMovement = true;
	bool bAttached = false;
	bool bFinishing = false;
	bool bStarting = false;
	FTimerHandle Watchdog;
	FTimerHandle SelectionTimer;
	FTimerHandle DashArrivalTimer;
	bool CanSelect(UGrabbableComponent* Candidate) const;
	float CalculateScale(const UGrabbableComponent* Candidate) const;
	void SetStage(EGurenQStage NewStage);
	void Finish();
	void FinishDash();
	UFUNCTION() void TargetDestroyed(AActor* DestroyedActor);
};
