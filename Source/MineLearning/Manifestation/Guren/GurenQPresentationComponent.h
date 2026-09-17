#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GurenQSkillComponent.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GurenQPresentationComponent.generated.h"

class UAnimMontage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCameraComponent;
class UPoseableMeshComponent;
class USpringArmComponent;
class APlayerController;

USTRUCT()
struct FGurenQAfterimage
{
	GENERATED_BODY()
	UPROPERTY() TObjectPtr<UPoseableMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
	double BornAt = 0.0;
};

/** Presentation subscribes to gameplay; Blueprint supplies the existing dissolve effect. */
UCLASS(Blueprintable, ClassGroup = (Guren), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UGurenQPresentationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGurenQPresentationComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Q Animation") TObjectPtr<UAnimMontage> DashMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Q Animation") TObjectPtr<UAnimMontage> GrabMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Q Effects") TObjectPtr<UMaterialInterface> AfterimageMaterial;
	UPROPERTY(EditAnywhere, Category = "Q Camera", meta = (ClampMin = "0", ClampMax = "2")) float ImpactStrength = 1.f;
	UPROPERTY(EditAnywhere, Category = "Q Camera", meta = (ClampMin = "0", ClampMax = "15")) float CloseUpFOV = 10.f;
	UPROPERTY(EditAnywhere, Category = "Q Camera", meta = (ClampMin = "0.1", ClampMax = "1")) float ContactTimeScale = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Q Camera", meta = (ClampMin = "0", ClampMax = "1")) float ContactSlowMotionDuration = 0.22f;
	UPROPERTY(EditAnywhere, Category = "Q Camera", meta = (ClampMin = "0", ClampMax = "1")) float DissolveShakeDuration = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Q Execution Camera") bool bUseExecutionCamera = true;
	UPROPERTY(EditAnywhere, Category = "Q Execution Camera", meta = (ClampMin = "300", ClampMax = "1800", Units = "cm")) float ExecutionCameraDistance = 750.f;
	UPROPERTY(EditAnywhere, Category = "Q Execution Camera", meta = (ClampMin = "-60", ClampMax = "60", Units = "deg")) float ExecutionCameraYawOffset = -20.f;
	UPROPERTY(EditAnywhere, Category = "Q Execution Camera", meta = (ClampMin = "-40", ClampMax = "10", Units = "deg")) float ExecutionCameraPitch = -10.f;
	UPROPERTY(EditAnywhere, Category = "Q Execution Camera", meta = (ClampMin = "-100", ClampMax = "200", Units = "cm")) float ExecutionCameraHeight = 60.f;
	UPROPERTY(EditAnywhere, Category = "Q Execution Camera", meta = (ClampMin = "0.1", ClampMax = "2", Units = "s")) float ExecutionCameraBlendIn = 0.7f;
	UPROPERTY(EditAnywhere, Category = "Q Execution Camera", meta = (ClampMin = "0.1", ClampMax = "2", Units = "s")) float ExecutionCameraBlendOut = 0.45f;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UFUNCTION(BlueprintImplementableEvent, Category = "Q Effects") void RadiationChanged(bool bActive, AActor* Target, FVector Origin);
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UPROPERTY() TObjectPtr<UGurenQSkillComponent> Skill;
	UPROPERTY() TObjectPtr<UAnimMontage> PlayingMontage;
	UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
	TWeakObjectPtr<USkeletalMeshComponent> RadiatingMesh;
	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<USpringArmComponent> CameraBoom;
	TWeakObjectPtr<APlayerController> CameraController;
	FRotator OriginalViewRotation;
	FRotator OrbitStartRotation;
	FRotator ExecutionViewRotation;
	FVector OriginalTargetOffset;
	FVector OrbitStartOffset;
	float OriginalArmLength = 0.f;
	float OrbitStartArmLength = 0.f;
	double OrbitStartedAt = 0.0;
	bool bExecutionCameraActive = false;
	bool bReturningCamera = false;
	UPROPERTY() TArray<FGurenQAfterimage> Afterimages;
	double LastRealTime = 0.0;
	double SlowMotionUntil = 0.0;
	double ShakeStartedAt = 0.0;
	double NextAfterimageAt = 0.0;
	float PreviousTimeDilation = 1.f;
	float ZoomOffset = 0.f;
	float ShakeAmplitude = 0.f;
	float ShakeDuration = 0.f;
	UFUNCTION() void StageChanged(EGurenQStage Stage, AActor* Target);
	UFUNCTION() void GrabContact();
	void BeginExecutionCamera();
	void ReturnExecutionCamera(bool bImmediate = false);
	void UpdateExecutionCamera(double Now);
	void RestoreTimeDilation();
	void SpawnAfterimage(double Now);
	void ClearAfterimages();
	void MontageEnded(UAnimMontage* Montage, bool bInterrupted);
};

UCLASS(meta = (DisplayName = "Guren Q Event"))
class MINELEARNING_API UAnimNotify_GurenQEvent : public UAnimNotify
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Q Skill") FName Event;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override { return Event.ToString(); }
};
