#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GrabbableComponent.generated.h"

class UPrimitiveComponent;
class UBrainComponent;

UENUM(BlueprintType)
enum class EGrabCompletion : uint8
{
	Destroy,
	Restore
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGrabStateChanged, AActor*, InstigatorActor, bool, bGrabbed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGrabCompleted, AActor*, InstigatorActor);

/** Place this scene component at the grip point. All targets share this reservation/attachment contract. */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UGrabbableComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	UGrabbableComponent();
	UFUNCTION(BlueprintPure, Category = "Grab") bool CanGrab(AActor* Requester) const;
	UFUNCTION(BlueprintPure, Category = "Grab") bool IsGrabbed() const { return Grabber.IsValid(); }
	UFUNCTION(BlueprintPure, Category = "Grab") float GetGripDiameter() const;
	UFUNCTION(BlueprintPure, Category = "Grab") FVector GetIndicatorLocation() const;
	bool Reserve(AActor* Requester);
	bool AttachToGrip(USceneComponent* Hand, FName Socket);
	void Release(bool bCompleted);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab") bool bGrabbable = true;
	/** Diameter of the part held by the claw, in local cm. Zero uses visible mesh bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab", meta = (ClampMin = "0", Units = "cm")) float GripDiameter = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab") EGrabCompletion Completion = EGrabCompletion::Destroy;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab", meta = (ClampMin = "0", Units = "cm")) float IndicatorHeight = 45.f;
	UPROPERTY(BlueprintAssignable, Category = "Grab") FGrabStateChanged OnGrabStateChanged;
	/** Target-owned gameplay can settle rewards or inventory here, before default completion. */
	UPROPERTY(BlueprintAssignable, Category = "Grab") FGrabCompleted OnGrabCompleted;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	struct FComponentState
	{
		TWeakObjectPtr<UActorComponent> Component;
		bool bTickEnabled = false;
		bool bSimulating = false;
		FVector LinearVelocity = FVector::ZeroVector;
		FVector AngularVelocity = FVector::ZeroVector;
	};
	TArray<FComponentState> ComponentStates;
	TWeakObjectPtr<AActor> Grabber;
	TWeakObjectPtr<USceneComponent> GripParent;
	FName GripSocket;
	FVector WorldGripOffset = FVector::ZeroVector;
	TWeakObjectPtr<UBrainComponent> PausedBrain;
	FTransform OriginalTransform;
	bool bActorTickEnabled = false;
	bool bCollisionEnabled = false;
	bool bAbsoluteLocation = false;
	bool bAbsoluteRotation = false;
	bool bAbsoluteScale = false;
	bool bReleasing = false;
	FBox GetMeshBounds() const;
};
