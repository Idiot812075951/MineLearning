#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GurenUltimateComponent.h"
#include "GurenUltimatePresentationComponent.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class UNiagaraComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USoundBase;
class ACineCameraActor;
class UDirectionalLightComponent;
class UPointLightComponent;

USTRUCT()
struct FArrivalMaterialSnapshot
{
	GENERATED_BODY()
	UPROPERTY() TWeakObjectPtr<UMeshComponent> Mesh;
	UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> Originals;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Instances;
	UPROPERTY() TObjectPtr<UNiagaraComponent> SurfaceEffect;
	UPROPERTY() TObjectPtr<UNiagaraComponent> PierceEffect;
	FVector Center = FVector::ZeroVector;
	float Radius = 1.f;
	float ParticleShare = 1.f;
	bool bBoundsFallback = false;
	FTransform RelativeTransform;
	int32 TargetIndex = INDEX_NONE;
	float ImpactAge = 0.f;
};

UCLASS(ClassGroup = (Guren), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UGurenUltimatePresentationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGurenUltimatePresentationComponent();
	UPROPERTY(EditAnywhere, Category = "Arrival|Animation") TObjectPtr<UAnimMontage> Montage;
	UPROPERTY(EditAnywhere, Category = "Arrival|Animation") FName LaunchBone = TEXT("radiant_forearm_r");
	UPROPERTY(EditAnywhere, Category = "Arrival|Assets") TObjectPtr<UStaticMesh> ArmMesh;
	UPROPERTY(EditAnywhere, Category = "Arrival|Assets") TObjectPtr<UStaticMesh> HaloMesh;
	UPROPERTY(EditAnywhere, Category = "Arrival|Assets") TObjectPtr<UMaterialInterface> HaloMaterial;
	UPROPERTY(EditAnywhere, Category = "Arrival|Assets") TObjectPtr<UNiagaraSystem> RadiationSystem;
	UPROPERTY(EditAnywhere, Category = "Arrival|Assets") TObjectPtr<UNiagaraSystem> StaticDisintegrateSystem;
	UPROPERTY(EditAnywhere, Category = "Arrival|Assets") TObjectPtr<UNiagaraSystem> SkeletalDisintegrateSystem;
	UPROPERTY(EditAnywhere, Category = "Arrival|Assets") TObjectPtr<UNiagaraSystem> StaticPierceSystem;
	UPROPERTY(EditAnywhere, Category = "Arrival|Assets") TObjectPtr<UNiagaraSystem> SkeletalPierceSystem;
	UPROPERTY(EditAnywhere, Category = "Arrival|Assets") TObjectPtr<UNiagaraSystem> TargetChargeSystem;
	/** Hidden sampling volume for grabbables without a supported visible mesh. */
	UPROPERTY(EditAnywhere, Category = "Arrival|Assets") TObjectPtr<UStaticMesh> BoundsSampleMesh;
	/** Particle budget is shared between every visible mesh on a target. */
	UPROPERTY(EditAnywhere, Category = "Arrival|Disintegration", meta=(ClampMin="30",ClampMax="3000")) int32 TargetParticleBudget = 1500;
	UPROPERTY(EditAnywhere, Category = "Arrival|Disintegration", meta=(ClampMin="0.1",ClampMax="0.8",Units="s")) float DisintegrateLeadTime = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Disintegration", meta=(ClampMin="0")) float DissolveHeatIntensity = 6.f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Pierce", meta=(ClampMin="0",ClampMax="200")) int32 PierceParticleBudget = 64;
	UPROPERTY(EditAnywhere, Category = "Arrival|Pierce", meta=(ClampMin="0.05",ClampMax="0.3",Units="s")) float PierceEmissionTime = 0.12f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Disintegration", meta=(ClampMin="0",ClampMax="1")) float SecondaryTargetIntensity = 0.7f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Disintegration", meta=(ClampMin="0")) float TargetHeatIntensity = 3.f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Audio") TArray<TObjectPtr<USoundBase>> BeatSounds;
	/** Along attack axis, side, height; wide framing uses the complete target group bounds. */
	UPROPERTY(EditAnywhere, Category = "Arrival|Camera") FVector EstablishOffset = FVector(-0.9f, 1.4f, 0.7f);
	UPROPERTY(EditAnywhere, Category = "Arrival|Camera") FVector ArrivalOffset = FVector(-1.45f, 0.75f, 0.14f);
	UPROPERTY(EditAnywhere, Category = "Arrival|Camera", meta=(ClampMin="30",ClampMax="100")) float FieldOfView = 64.f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Camera", meta=(ClampMin="30",ClampMax="100")) float WideFieldOfView = 76.f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Camera") float CameraBlendTime = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Camera", meta=(ClampMin="300",Units="cm")) float HeroCameraDistance = 1100.f;
	/** Moves the final framing focus toward the heated targets while retaining Guren in the foreground. */
	UPROPERTY(EditAnywhere, Category = "Arrival|Camera", meta=(ClampMin="0",ClampMax="1")) float ArrivalTargetFramingWeight = 0.3f;
	/** Caps the pullback so scattered targets cannot turn the hero shot into a distant overview. */
	UPROPERTY(EditAnywhere, Category = "Arrival|Camera", meta=(ClampMin="300",Units="cm")) float ArrivalMaxCameraDistance = 2200.f;
	UPROPERTY(EditAnywhere, Category = "Arrival|VFX") float HaloRadius = 330.f;
	UPROPERTY(EditAnywhere, Category = "Arrival|VFX") FVector HaloOffset = FVector(-85.f, 0.f, 45.f);
	UPROPERTY(EditAnywhere, Category = "Arrival|VFX", meta=(ClampMin="8",ClampMax="128")) int32 TrailSegments = 64;
	UPROPERTY(EditAnywhere, Category = "Arrival|VFX", meta=(ClampMin="1",Units="cm")) float TrailWidth = 28.f;
	/** World-space trail length, independent of speed and frame rate. */
	UPROPERTY(EditAnywhere, Category = "Arrival|VFX", meta=(ClampMin="1",Units="cm")) float TrailLength = 350.f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Night") bool bNightAtmosphere = true;
	UPROPERTY(EditAnywhere, Category = "Arrival|Night") TObjectPtr<UStaticMesh> NightSkyMesh;
	UPROPERTY(EditAnywhere, Category = "Arrival|Night") TObjectPtr<UMaterialInterface> NightSkyMaterial;
	UPROPERTY(EditAnywhere, Category = "Arrival|Night", meta=(ClampMin="0",ClampMax="1")) float NightSunMultiplier = 0.38f;
	UPROPERTY(EditAnywhere, Category = "Arrival|Night") float HeroLightIntensity = 180000.f;
	UFUNCTION(BlueprintPure, Category = "Arrival") int32 GetCameraPreset() const { return CameraPreset; }
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	struct FSunState
	{
		TWeakObjectPtr<UDirectionalLightComponent> Light;
		float Intensity = 0.f;
		FLinearColor Color;
	};
	UPROPERTY() TObjectPtr<UGurenUltimateComponent> Skill;
	UPROPERTY() TObjectPtr<ACineCameraActor> Camera;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Arm;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Halo;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> NightSky;
	UPROPERTY() TObjectPtr<UPointLightComponent> HeroLight;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> HaloMID;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Trails;
	UPROPERTY() TArray<TObjectPtr<UNiagaraComponent>> TargetCharges;
	UPROPERTY() TArray<TObjectPtr<UNiagaraComponent>> Effects;
	UPROPERTY() TArray<FArrivalMaterialSnapshot> Materials;
	TArray<FSunState> Suns;
	TArray<FVector> TrailHistory;
	TWeakObjectPtr<APlayerController> Player;
	TWeakObjectPtr<AActor> OriginalViewTarget;
	FVector ArmPalmOffset = FVector::ZeroVector;
	FQuat ArmRotationOffset = FQuat::Identity;
	float Age = 0.f;
	float BurstAge = -1.f;
	float CameraSide = 1.f;
	int32 CameraPreset = 0;
	bool bArmHidden = false;
	UFUNCTION() void StageChanged(EGurenUltimateStage Stage);
	UFUNCTION() void TargetPierced(int32 TargetIndex);
	void MontageEnded(UAnimMontage* EndedMontage, bool bInterrupted);
	void Cleanup();
	void UpdateTargets(float DeltaTime = 0.f);
	void UpdateCamera(float DeltaTime);
	void GetArrivalView(const FVector& GurenLocation, FVector& Look, FVector& Position) const;
	void UpdateArm(float DeltaTime);
	void EnterNight();
	UStaticMeshComponent* MakePlane(UMaterialInterface* Material);
	UNiagaraComponent* SpawnRadiation(const FVector& Location, float Scale);
};
