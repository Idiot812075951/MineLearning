#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GurenQSkillComponent.generated.h"

class AQGrabTestDummy;
class ACharacter;
class AController;

UENUM(BlueprintType)
enum class EGurenQStage : uint8
{
	Idle, Dash, Grab, Radiation, Release
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGurenQStageChanged, EGurenQStage, Stage, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGurenQContact);

/** Target selection and grab lifecycle. Animation/VFX subscribe to the stage event. */
UCLASS(ClassGroup = (Guren), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UGurenQSkillComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGurenQSkillComponent();
	UFUNCTION(BlueprintCallable, Category = "Q Skill") void TryCast();
	UFUNCTION(BlueprintCallable, Category = "Q Skill") void HandleAnimationEvent(FName Event);
	UFUNCTION(BlueprintCallable, Category = "Q Skill") void Cancel();
	UFUNCTION(BlueprintPure, Category = "Q Skill") bool IsQActive() const { return Stage != EGurenQStage::Idle; }
	UFUNCTION(BlueprintPure, Category = "Q Skill") EGurenQStage GetStage() const { return Stage; }
	UFUNCTION(BlueprintPure, Category = "Q Skill") AActor* GetTarget() const;
	UPROPERTY(BlueprintAssignable, Category = "Q Skill") FGurenQStageChanged OnStageChanged;
	UPROPERTY(BlueprintAssignable, Category = "Q Skill") FGurenQContact OnGrabContact;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UPROPERTY(EditAnywhere, Category = "Q Skill") float SelectionRange = 2000.f;
	UPROPERTY(EditAnywhere, Category = "Q Skill") float DirectGrabRange = 200.f;
	UPROPERTY(EditAnywhere, Category = "Q Skill") FName GripSocket = TEXT("Q_GrabHead");
	EGurenQStage Stage = EGurenQStage::Idle;
	TWeakObjectPtr<AQGrabTestDummy> Target;
	TWeakObjectPtr<AController> LockedController;
	FTransform OriginalTargetTransform;
	FTransform StandTransform;
	bool bOriginalCollision = true;
	bool bOriginalOrientToMovement = true;
	bool bAttached = false;
	FTimerHandle Watchdog;
	void SetStage(EGurenQStage NewStage);
	void Finish(bool bCancelled);
	void FinishDash();
	UFUNCTION() void TargetDestroyed(AActor* DestroyedActor);
};
