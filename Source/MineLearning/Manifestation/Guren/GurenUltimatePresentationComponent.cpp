#include "GurenUltimatePresentationComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MineLearning/Effects/MaterialEffectLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

UGurenUltimatePresentationComponent::UGurenUltimatePresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UGurenUltimatePresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	Skill = GetOwner()->FindComponentByClass<UGurenUltimateComponent>();
	if (Skill)
	{
		Skill->OnStageChanged.AddDynamic(this, &UGurenUltimatePresentationComponent::StageChanged);
		Skill->OnTargetPierced.AddDynamic(this, &UGurenUltimatePresentationComponent::TargetPierced);
	}
}

UNiagaraComponent* UGurenUltimatePresentationComponent::SpawnRadiation(const FVector& Location, float Scale)
{
	if (!RadiationSystem)
	{
		return nullptr;
	}
	UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), RadiationSystem, Location,
		FRotator::ZeroRotator, FVector(Scale), false, true, ENCPoolMethod::None, false);
	if (Effect)
	{
		Effects.Add(Effect);
		Effect->SetVariableFloat(TEXT("User.Intensity"), 0.65f);
		Effect->SetVariableFloat(TEXT("User.HeatIntensity"), 0.5f);
	}
	return Effect;
}

UStaticMeshComponent* UGurenUltimatePresentationComponent::MakePlane(UMaterialInterface* Material)
{
	if (!HaloMesh || !Material)
	{
		return nullptr;
	}
	UStaticMeshComponent* Plane = NewObject<UStaticMeshComponent>(GetOwner());
	Plane->SetStaticMesh(HaloMesh);
	Plane->SetMaterial(0, Material);
	Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plane->SetCastShadow(false);
	Plane->RegisterComponent();
	return Plane;
}

void UGurenUltimatePresentationComponent::EnterNight()
{
	if (!bNightAtmosphere)
	{
		return;
	}
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		if (UDirectionalLightComponent* Light = It->FindComponentByClass<UDirectionalLightComponent>())
		{
			FSunState& State = Suns.AddDefaulted_GetRef();
			State.Light = Light;
			State.Intensity = Light->Intensity;
			State.Color = Light->GetLightColor();
		}
	}
	if (NightSkyMesh && NightSkyMaterial)
	{
		NightSky = NewObject<UStaticMeshComponent>(GetOwner());
		NightSky->SetStaticMesh(NightSkyMesh);
		NightSky->SetMaterial(0, NightSkyMaterial);
		NightSky->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NightSky->SetCastShadow(false);
		NightSky->RegisterComponent();
		NightSky->SetWorldLocation(GetOwner()->GetActorLocation());
		NightSky->SetWorldScale3D(FVector(400.f));
	}
	HeroLight = NewObject<UPointLightComponent>(GetOwner());
	HeroLight->SetLightColor(FLinearColor(0.65f, 0.78f, 1.f));
	HeroLight->SetIntensity(HeroLightIntensity);
	HeroLight->SetAttenuationRadius(2400.f);
	HeroLight->SetCastShadows(false);
	HeroLight->RegisterComponent();
}

void UGurenUltimatePresentationComponent::TargetPierced(int32 TargetIndex)
{
	if (!Skill->GetTargets().IsValidIndex(TargetIndex))
	{
		return;
	}
	const FArrivalTarget& Target = Skill->GetTargets()[TargetIndex];
	if (AActor* Actor = Target.Actor.Get())
	{
		TInlineComponentArray<UMeshComponent*> Meshes(Actor);
		const int32 FirstSnapshot = Materials.Num();
		UStaticMeshComponent* BoundsSource = nullptr;
		const auto IsSurface = [](const UMeshComponent* Mesh)
		{
			const USkeletalMeshComponent* Skeletal = Cast<USkeletalMeshComponent>(Mesh);
			const UStaticMeshComponent* Static = Cast<UStaticMeshComponent>(Mesh);
			return Mesh->IsVisible() && !Mesh->bHiddenInGame && !Mesh->IsEditorOnly()
				&& ((Skeletal && Skeletal->GetSkeletalMeshAsset()) || (Static && Static->GetStaticMesh()));
		};
		if (!Meshes.ContainsByPredicate(IsSurface) && BoundsSampleMesh)
		{
			FBox Bounds = Actor->GetComponentsBoundingBox(true);
			if (!Bounds.IsValid || Bounds.GetExtent().IsNearlyZero())
			{
				Bounds = FBox(Target.Location - FVector(Target.Radius), Target.Location + FVector(Target.Radius));
			}
			BoundsSource = NewObject<UStaticMeshComponent>(GetOwner());
			BoundsSource->SetStaticMesh(BoundsSampleMesh);
			BoundsSource->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			BoundsSource->SetCastShadow(false);
			BoundsSource->SetVisibility(false);
			BoundsSource->RegisterComponent();
			BoundsSource->SetWorldLocation(Bounds.GetCenter());
			BoundsSource->SetWorldScale3D(Bounds.GetExtent().ComponentMax(FVector(10.f)) / 50.f);
			Meshes.Add(BoundsSource);
		}
		for (UMeshComponent* Mesh : Meshes)
		{
			if (Mesh != BoundsSource && !IsSurface(Mesh))
			{
				continue;
			}
			FArrivalMaterialSnapshot& Snapshot = Materials.AddDefaulted_GetRef();
			Snapshot.Mesh = Mesh;
			Snapshot.TargetIndex = TargetIndex;
			Snapshot.ImpactAge = Age;
			Snapshot.RelativeTransform = Mesh->GetRelativeTransform();
			Snapshot.Center = Mesh->Bounds.Origin;
			Snapshot.Radius = FMath::Max(10.f, static_cast<float>(Mesh->Bounds.SphereRadius));
			Snapshot.bBoundsFallback = Mesh == BoundsSource;
			for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
			{
				Snapshot.Originals.Add(Mesh->GetMaterial(Index));
				Snapshot.Instances.Add(UMaterialEffectLibrary::CreateIsolatedMaterialInstance(Mesh, Index));
			}
			const float Size = FMath::Clamp(Snapshot.Radius / 100.f, 0.8f, 3.f);
			const auto CreateSampler = [&](UNiagaraSystem* System, float Speed, float Duration) -> UNiagaraComponent*
			{
				if (!System)
				{
					return nullptr;
				}
				UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System,
					Snapshot.Center, FRotator::ZeroRotator, FVector::OneVector, false, false, ENCPoolMethod::None, false);
				if (!Effect)
				{
					return nullptr;
				}
				if (USkeletalMeshComponent* Skeletal = Cast<USkeletalMeshComponent>(Mesh))
				{
					UNiagaraFunctionLibrary::OverrideSystemUserVariableSkeletalMeshComponent(Effect, TEXT("User.TargetSkeletalMesh"), Skeletal);
				}
				else
				{
					UNiagaraFunctionLibrary::OverrideSystemUserVariableStaticMeshComponent(Effect, TEXT("User.TargetMesh"), CastChecked<UStaticMeshComponent>(Mesh));
				}
				Effect->SetVariableFloat(TEXT("User.SpawnRate"), 0.f);
				Effect->SetVariableFloat(TEXT("User.ParticleScale"), Size);
				Effect->SetVariableFloat(TEXT("User.ParticleSpeed"), Speed * FMath::Sqrt(Size));
				Effect->SetVariableFloat(TEXT("User.EffectDuration"), Duration);
				Effect->SetVariableFloat(TEXT("User.InwardDepth"), Snapshot.bBoundsFallback ? Mesh->Bounds.BoxExtent.GetMin() : 2.f);
				Effect->SetVariablePosition(TEXT("User.ScatterCenterWS"), Snapshot.Center - Skill->GetAttackDirection() * Snapshot.Radius * 0.35f);
				Effect->SetVariablePosition(TEXT("User.DissolveOriginWS"), Snapshot.Center);
				Effect->Activate(true);
				return Effect;
			};
			const bool bSkeletal = Mesh->IsA<USkeletalMeshComponent>();
			Snapshot.PierceEffect = CreateSampler(bSkeletal ? SkeletalPierceSystem : StaticPierceSystem, 440.f, 1.f);
			// Use Q's surface sampler and its 6-10% lifetime convention for the final white disintegration.
			Snapshot.SurfaceEffect = CreateSampler(bSkeletal ? SkeletalDisintegrateSystem : StaticDisintegrateSystem, 45.f, 3.f);
		}
		float TotalArea = 0.f;
		for (int32 Index = FirstSnapshot; Index < Materials.Num(); ++Index)
		{
			TotalArea += FMath::Square(Materials[Index].Radius);
		}
		for (int32 Index = FirstSnapshot; Index < Materials.Num(); ++Index)
		{
			FArrivalMaterialSnapshot& Snapshot = Materials[Index];
			Snapshot.ParticleShare = FMath::Square(Snapshot.Radius) / FMath::Max(1.f, TotalArea);
			if (Snapshot.PierceEffect)
			{
				const float Weight = TargetIndex == 0 ? 1.f : SecondaryTargetIntensity;
				Snapshot.PierceEffect->SetVariableFloat(TEXT("User.SpawnRate"),
					PierceParticleBudget * Weight * Snapshot.ParticleShare / FMath::Max(0.05f, PierceEmissionTime));
			}
		}
		if (TargetChargeSystem && Materials.Num() > FirstSnapshot && TargetCharges.IsValidIndex(TargetIndex))
		{
			FBox Bounds(ForceInit);
			for (int32 Index = FirstSnapshot; Index < Materials.Num(); ++Index)
			{
				Bounds += Materials[Index].Mesh->Bounds.GetBox();
			}
			TargetCharges[TargetIndex] = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TargetChargeSystem,
				Bounds.GetCenter(), FRotator::ZeroRotator, FVector(FMath::Clamp(Bounds.GetExtent().GetMax() / 45.f, 0.7f, 8.f)),
				false, true, ENCPoolMethod::None, false);
		}
	}
	if (BeatSounds.IsValidIndex(2) && BeatSounds[2])
	{
		UGameplayStatics::PlaySoundAtLocation(this, BeatSounds[2], Target.Location, 0.7f);
	}
}

void UGurenUltimatePresentationComponent::UpdateTargets(float DeltaTime)
{
	const EGurenUltimateStage Stage = Skill->GetStage();
	const float Remaining = Stage == EGurenUltimateStage::Arrival ? Skill->GetStageDuration(Stage) - Skill->GetStageTime() : DisintegrateLeadTime + 1.f;
	const float Dissolve = Stage >= EGurenUltimateStage::Burst ? 1.f : FMath::Clamp(1.f - Remaining / FMath::Max(0.1f, DisintegrateLeadTime), 0.f, 1.f);
	const bool bWhiteDissolve = Dissolve > 0.f;
	for (const FArrivalMaterialSnapshot& Snapshot : Materials)
	{
		const float Heat = FMath::Clamp((Age - Snapshot.ImpactAge) / 2.4f, 0.f, 1.f);
		const float Progress = bWhiteDissolve ? FMath::Lerp(0.8f, 1.f, Dissolve) : FMath::Lerp(0.16f, 0.76f, Heat);
		const FVector Origin = Snapshot.Center;
		const float Radius = Snapshot.Radius * 1.15f;
		const float Weight = Snapshot.TargetIndex == 0 ? 1.f : SecondaryTargetIntensity;
		if (UNiagaraComponent* Effect = Snapshot.PierceEffect)
		{
			// Integrate the portion of this frame inside the hit window, so a long frame cannot skip it entirely.
			const float HitAge = Age - Snapshot.ImpactAge;
			const float Duration = FMath::Max(0.05f, PierceEmissionTime);
			const float EmittingTime = FMath::Clamp(HitAge, 0.f, Duration) - FMath::Clamp(HitAge - DeltaTime, 0.f, Duration);
			Effect->SetVariableFloat(TEXT("User.SpawnRate"), Snapshot.Mesh.IsValid() && DeltaTime > 0.f
				? PierceParticleBudget * Weight * Snapshot.ParticleShare * EmittingTime / (Duration * DeltaTime) : 0.f);
		}
		if (UNiagaraComponent* Effect = Snapshot.SurfaceEffect)
		{
			// The GPU samples while the source still exists, before synchronous gameplay settlement.
			const bool bEmitting = Snapshot.Mesh.IsValid() && Dissolve > 0.f && Dissolve < 1.f;
			Effect->SetVariableFloat(TEXT("User.SpawnRate"), bEmitting ? TargetParticleBudget * Weight * Snapshot.ParticleShare / FMath::Max(0.1f, DisintegrateLeadTime) : 0.f);
			Effect->SetVariableFloat(TEXT("User.DissolveRadius"), Radius * Dissolve);
			Effect->SetVariableFloat(TEXT("User.FrontWidth"), Radius * 0.35f);
		}
		for (UMaterialInstanceDynamic* MID : Snapshot.Instances)
		{
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("DissolveOriginWS"), FLinearColor(Origin.X, Origin.Y, Origin.Z));
				MID->SetScalarParameterValue(TEXT("DissolveProgress"), Progress);
				MID->SetScalarParameterValue(TEXT("HeatRadius"), Radius * FMath::Lerp(0.35f, 1.5f, Heat));
				MID->SetScalarParameterValue(TEXT("DissolveRadius"), Dissolve > 0.f ? Radius * Dissolve : -10.f);
				MID->SetScalarParameterValue(TEXT("DissolveEdgeWidth"), bWhiteDissolve ? 3.f : 5.f);
				MID->SetScalarParameterValue(TEXT("HeatIntensity"), Weight * (bWhiteDissolve ? DissolveHeatIntensity : TargetHeatIntensity * (0.2f + Heat)));
				// Q's original heat/edge branch stays active throughout the visible dissolve, not a single white flash.
				MID->SetScalarParameterValue(TEXT("RadiantStress"), bWhiteDissolve ? 0.f : Heat);
				MID->SetScalarParameterValue(TEXT("RadiantCritical"), 0.f);
			}
		}
		if (UMeshComponent* Mesh = Snapshot.Mesh.Get())
		{
			const float HitTime = Age - Snapshot.ImpactAge;
			const float Recoil = HitTime < 0.3f ? FMath::Sin(HitTime / 0.3f * UE_PI) * 5.f : 0.f;
			FTransform Transform = Snapshot.RelativeTransform;
			Transform.ConcatenateRotation(FRotator(Recoil, 0.f, 0.f).Quaternion());
			Mesh->SetRelativeTransform(Transform);
		}
	}
	for (int32 Index = 0; Index < TargetCharges.Num(); ++Index)
	{
		if (UNiagaraComponent* Charge = TargetCharges[Index])
		{
			const float Tail = BurstAge < 0.f ? 1.f : FMath::Max(0.f, 1.f - (Age - BurstAge) / 0.25f);
			const float Weight = Index == 0 ? 1.f : SecondaryTargetIntensity;
			Charge->SetVariableFloat(TEXT("User.Intensity"), Weight * Tail * (Stage >= EGurenUltimateStage::Arrival ? 0.8f : 0.3f));
			Charge->SetVariableFloat(TEXT("User.HeatIntensity"), Weight * Tail * (0.25f + Dissolve * 0.65f));
		}
	}
}

void UGurenUltimatePresentationComponent::StageChanged(EGurenUltimateStage Stage)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (Stage == EGurenUltimateStage::Idle)
	{
		Cleanup();
		return;
	}
	if (Stage == EGurenUltimateStage::Ready)
	{
		Age = 0.f;
		BurstAge = -1.f;
		TargetCharges.SetNum(Skill->GetTargets().Num());
		Player = Cast<APlayerController>(Character->GetController());
		OriginalViewTarget = Player.IsValid() ? Player->GetViewTarget() : nullptr;
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Camera = GetWorld()->SpawnActor<ACineCameraActor>(Params);
		UCineCameraComponent* Lens = Camera->GetCineCameraComponent();
		Lens->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
		Lens->SetCurrentFocalLength(Lens->Filmback.SensorWidth * 0.5f / FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5f)));
		Lens->PostProcessSettings.bOverride_VignetteIntensity = true;
		Lens->PostProcessSettings.VignetteIntensity = 0.25f;
		Lens->PostProcessSettings.bOverride_MotionBlurAmount = true;
		Lens->PostProcessSettings.MotionBlurAmount = 0.18f;
		if (bNightAtmosphere)
		{
			Lens->PostProcessSettings.bOverride_ColorGain = true;
			Lens->PostProcessSettings.ColorGain = FVector4(0.85f, 0.9f, 1.05f, 1.f);
			Lens->PostProcessSettings.bOverride_AutoExposureBias = true;
			Lens->PostProcessSettings.AutoExposureBias = 0.f;
		}
		CameraSide = 1.f;
		CameraPreset = 0;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(ArrivalCameraSelection), false, Character);
		for (const FArrivalTarget& Target : Skill->GetTargets())
		{
			Query.AddIgnoredActor(Target.Actor.Get());
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			CameraSide = Index == 1 ? -1.f : 1.f;
			CameraPreset = Index;
			FVector Focus, Position;
			GetArrivalView(Skill->GetArrivalLocation(), Focus, Position);
			if (Index == 2 || !GetWorld()->LineTraceTestByChannel(Position, Focus, ECC_Visibility, Query))
			{
				break;
			}
		}
		UpdateCamera(0.f);
		if (Player.IsValid())
		{
			Player->SetViewTargetWithBlend(Camera, CameraBlendTime);
		}
		EnterNight();
		SpawnRadiation(Mesh->GetSocketLocation(TEXT("socket_palm_fx")), 0.7f);
		if (HaloMaterial)
		{
			HaloMID = UMaterialInstanceDynamic::Create(HaloMaterial, this);
			// The hero corona has its own material mode; impact rings and arm trails retain their shapes.
			HaloMID->SetScalarParameterValue(TEXT("Style"), 2.f);
			Halo = MakePlane(HaloMID);
			if (Halo)
			{
				Halo->SetVisibility(false);
			}
		}
		SetComponentTickEnabled(true);
	}
	if (UAnimInstance* Anim = Mesh->GetAnimInstance(); Anim && Montage)
	{
		// Sections are presentation clips; changing gameplay durations automatically adjusts their playback rate.
		static const FName Sections[] = { TEXT("Ready"), TEXT("Launch"), TEXT("Impact"), TEXT("Descent"), TEXT("Arrival"), TEXT("Burst"), TEXT("Recover") };
		const FName Section = Sections[static_cast<int32>(Stage) - 1];
		const int32 Index = Montage->GetSectionIndex(Section);
		if (!Anim->Montage_IsActive(Montage))
		{
			Anim->Montage_Play(Montage);
			FOnMontageEnded End;
			End.BindUObject(this, &UGurenUltimatePresentationComponent::MontageEnded);
			Anim->Montage_SetEndDelegate(End, Montage);
		}
		if (Index != INDEX_NONE)
		{
			Anim->Montage_SetPlayRate(Montage, Montage->GetSectionLength(Index) / Skill->GetStageDuration(Stage));
			Anim->Montage_JumpToSection(Section, Montage);
		}
	}
	if (Stage == EGurenUltimateStage::Launch && ArmMesh)
	{
		const FTransform LaunchTransform = Mesh->GetSocketTransform(LaunchBone);
		const FVector PalmLocation = Mesh->GetSocketLocation(TEXT("socket_palm_fx"));
		ArmPalmOffset = LaunchTransform.InverseTransformPosition(PalmLocation);
		ArmRotationOffset = FQuat::FindBetweenNormals(ArmPalmOffset.GetSafeNormal(), FVector::ForwardVector);
		Arm = NewObject<UStaticMeshComponent>(GetOwner());
		Arm->SetStaticMesh(ArmMesh);
		Arm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Arm->RegisterComponent();
		Arm->SetWorldTransform(LaunchTransform);
		Arm->SetVisibility(false);
		Mesh->HideBoneByName(LaunchBone, EPhysBodyOp::PBO_None);
		bArmHidden = true;
		TrailHistory.Add(LaunchTransform.GetLocation());
		if (HaloMaterial)
		{
			UMaterialInstanceDynamic* TrailMaterial = UMaterialInstanceDynamic::Create(HaloMaterial, this);
			TrailMaterial->SetScalarParameterValue(TEXT("Style"), 1.f);
			TrailMaterial->SetScalarParameterValue(TEXT("Intensity"), 4.f);
			for (int32 Index = 0; Index < FMath::Clamp(TrailSegments, 8, 128); ++Index)
			{
				if (UStaticMeshComponent* Trail = MakePlane(TrailMaterial))
				{
					Trail->SetVisibility(false);
					Trails.Add(Trail);
				}
			}
		}
	}
	else if (Stage == EGurenUltimateStage::Descent)
	{
		Mesh->UnHideBoneByName(LaunchBone);
		bArmHidden = false;
		if (Arm)
		{
			Arm->DestroyComponent();
			Arm = nullptr;
		}
		for (UStaticMeshComponent* Trail : Trails)
		{
			Trail->SetVisibility(false);
		}
		if (!Effects.IsEmpty())
		{
			Effects[0]->SetVisibility(false);
		}
	}
	else if (Stage == EGurenUltimateStage::Burst)
	{
		BurstAge = Age;
		UpdateTargets();
	}
	else if (Stage == EGurenUltimateStage::Recover && Player.IsValid() && OriginalViewTarget.IsValid())
	{
		Player->SetViewTargetWithBlend(OriginalViewTarget.Get(), CameraBlendTime);
	}
	const int32 SoundIndex = static_cast<int32>(Stage) - 1;
	if (Stage != EGurenUltimateStage::Impact && BeatSounds.IsValidIndex(SoundIndex) && BeatSounds[SoundIndex])
	{
		UGameplayStatics::PlaySoundAtLocation(this, BeatSounds[SoundIndex], GetOwner()->GetActorLocation());
	}
}

void UGurenUltimatePresentationComponent::GetArrivalView(const FVector& GurenLocation, FVector& Look, FVector& Position) const
{
	const FVector Hero = GurenLocation + FVector(0, 0, 45);
	const FVector Targets = Skill->GetTargetBounds().GetCenter();
	const FVector Axis = Skill->GetAttackDirection();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Axis) * CameraSide;
	FVector Offset = CameraPreset == 2 ? FVector(-1.f, 0.f, 0.75f) : ArrivalOffset;
	// Elevate the camera with the hero-to-target sightline, so ground targets stay behind the airborne pose.
	const float Slope = (Hero.Z - Targets.Z) / FMath::Max(600.f, static_cast<float>(FVector::Dist2D(Hero, Targets)));
	Offset.Z = FMath::Max(Offset.Z, FMath::Abs(Offset.X) * FMath::Clamp(Slope, 0.f, 0.65f));
	const FVector Outward = (Axis * Offset.X + Side * Offset.Y + FVector::UpVector * Offset.Z).GetSafeNormal();
	Look = FMath::Lerp(Hero, Targets, FMath::Clamp(ArrivalTargetFramingWeight, 0.f, 1.f));
	const FRotationMatrix ViewAxes((-Outward).Rotation());
	const FVector Right = ViewAxes.GetUnitAxis(EAxis::Y);
	const FVector Up = ViewAxes.GetUnitAxis(EAxis::Z);
	// CineCamera's filmback defines the vertical framing; a wide editor viewport must not inflate the pullback.
	const float Aspect = Camera->GetCineCameraComponent()->AspectRatio;
	const float TanHorizontal = FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5f)) * 0.88f;
	const float TanVertical = TanHorizontal / FMath::Max(1.f, Aspect);
	float Distance = HeroCameraDistance;
	const auto IncludeBounds = [&](const FBox& Bounds)
	{
		for (int32 Corner = 0; Corner < 8; ++Corner)
		{
			const FVector Point(Corner & 1 ? Bounds.Max.X : Bounds.Min.X,
				Corner & 2 ? Bounds.Max.Y : Bounds.Min.Y, Corner & 4 ? Bounds.Max.Z : Bounds.Min.Z);
			const FVector Relative = Point - Look;
			const float Depth = FVector::DotProduct(Relative, -Outward);
			Distance = FMath::Max(Distance, static_cast<float>(FMath::Max(
				FMath::Abs(FVector::DotProduct(Relative, Right)) / TanHorizontal,
				FMath::Abs(FVector::DotProduct(Relative, Up)) / TanVertical) - Depth));
		}
	};
	IncludeBounds(FBox(Hero - FVector(HaloRadius), Hero + FVector(HaloRadius)));
	for (const FArrivalTarget& Target : Skill->GetTargets())
	{
		// Cached bounds keep the shot stable when settlement destroys its subjects.
		IncludeBounds(FBox(Target.Location - FVector(Target.Radius), Target.Location + FVector(Target.Radius)));
	}
	Position = Look + Outward * FMath::Min(Distance, FMath::Max(HeroCameraDistance, ArrivalMaxCameraDistance));
}

void UGurenUltimatePresentationComponent::UpdateCamera(float DeltaTime)
{
	if (!Camera)
	{
		return;
	}
	const EGurenUltimateStage Stage = Skill->GetStage();
	const FVector Guren = GetOwner()->GetActorLocation();
	const FVector Axis = Skill->GetAttackDirection();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Axis) * CameraSide;
	FBox Group = Skill->GetTargetBounds();
	Group += Guren + FVector(0, 0, 250);
	Group += Guren - FVector(0, 0, 250);
	Group = Group.ExpandBy(FVector(Skill->ArcWidth * 0.3f, Skill->ArcWidth * 0.3f, 0.f));
	const FVector WideLook = Group.GetCenter();
	const float Span = FMath::Max(1800.f, static_cast<float>(Group.GetExtent().Size()));
	const FVector WidePosition = WideLook + (Axis * EstablishOffset.X + Side * EstablishOffset.Y + FVector::UpVector * EstablishOffset.Z) * Span;
	FVector HeroLook, HeroPosition;
	GetArrivalView(Guren, HeroLook, HeroPosition);
	const float Blend = Stage > EGurenUltimateStage::Descent ? 1.f : Stage == EGurenUltimateStage::Descent
		? FMath::SmoothStep(0.f, 0.85f, Skill->GetStageTime() / Skill->GetStageDuration(Stage)) : 0.f;
	const FVector Look = FMath::Lerp(WideLook, HeroLook, Blend);
	UCineCameraComponent* Lens = Camera->GetCineCameraComponent();
	const float FOV = FMath::Lerp(WideFieldOfView, FieldOfView, Blend);
	Lens->SetCurrentFocalLength(Lens->Filmback.SensorWidth * 0.5f / FMath::Tan(FMath::DegreesToRadians(FOV * 0.5f)));
	FVector Desired = FMath::Lerp(WidePosition, HeroPosition, Blend);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ArrivalCameraCollision), false, GetOwner());
	for (const FArrivalTarget& Target : Skill->GetTargets())
	{
		Query.AddIgnoredActor(Target.Actor.Get());
	}
	FHitResult Wall;
	if (GetWorld()->SweepSingleByChannel(Wall, Look, Desired, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(16.f), Query))
	{
		Desired = Wall.Location + Wall.Normal * 20.f;
	}
	const FVector Position = DeltaTime > 0.f ? FMath::VInterpTo(Camera->GetActorLocation(), Desired, DeltaTime, 7.f) : Desired;
	FRotator Rotation = (Look - Position).Rotation();
	if (Stage == EGurenUltimateStage::Burst)
	{
		Rotation.Pitch += 0.45f * FMath::Max(0.f, 1.f - Skill->GetStageTime() / 0.6f) * FMath::Sin(Skill->GetStageTime() * 28.f);
	}
	Camera->SetActorLocationAndRotation(Position, Rotation);
}

void UGurenUltimatePresentationComponent::UpdateArm(float DeltaTime)
{
	if (!Arm)
	{
		return;
	}
	USkeletalMeshComponent* Mesh = CastChecked<ACharacter>(GetOwner())->GetMesh();
	const int32 BoneIndex = Mesh->GetBoneIndex(LaunchBone);
	const TArray<uint8>& Visibility = Mesh->GetBoneVisibilityStates();
	const bool bHidden = Visibility.IsValidIndex(BoneIndex) && Visibility[BoneIndex] != BVS_Visible;
	Arm->SetVisibility(bHidden);
	const FVector Previous = Arm->GetComponentLocation();
	const FVector Position = Skill->GetFlightLocation();
	Arm->SetWorldLocation(Position);
	if (!Position.Equals(Previous, 0.1f))
	{
		const FQuat Rotation = (Position - Previous).Rotation().Quaternion() * ArmRotationOffset;
		Arm->SetWorldRotation(FMath::QInterpTo(Arm->GetComponentQuat(), Rotation, DeltaTime, 20.f));
	}
	if (TrailHistory.IsEmpty() || FVector::DistSquared(TrailHistory.Last(), Position) > FMath::Square(35.f))
	{
		TrailHistory.Add(Position);
		if (TrailHistory.Num() > Trails.Num() + 1)
		{
			TrailHistory.RemoveAt(0);
		}
	}
	float Length = 0.f;
	const float MaxLength = FMath::Max(1.f, TrailLength);
	for (int32 Index = TrailHistory.Num() - 1; Index > 0; --Index)
	{
		const float SegmentLength = FVector::Dist(TrailHistory[Index], TrailHistory[Index - 1]);
		if (Length + SegmentLength > MaxLength)
		{
			TrailHistory[Index - 1] = FMath::Lerp(TrailHistory[Index], TrailHistory[Index - 1], (MaxLength - Length) / SegmentLength);
			TrailHistory.RemoveAt(0, Index - 1);
			break;
		}
		Length += SegmentLength;
	}
	for (int32 Index = 0; Index < Trails.Num(); ++Index)
	{
		UStaticMeshComponent* Trail = Trails[Index];
		const bool bValid = bHidden && TrailHistory.IsValidIndex(Index + 1);
		Trail->SetVisibility(bValid);
		if (bValid && Camera)
		{
			const FVector Start = TrailHistory[Index];
			const FVector End = TrailHistory[Index + 1];
			Trail->SetWorldLocation((Start + End) * 0.5f);
			Trail->SetWorldRotation(FRotationMatrix::MakeFromXZ((End - Start).GetSafeNormal(), (Camera->GetActorLocation() - End).GetSafeNormal()).Rotator());
			Trail->SetWorldScale3D(FVector(FVector::Dist(Start, End) / 100.f, TrailWidth / 100.f * FMath::Lerp(0.2f, 1.f, static_cast<float>(Index + 1) / (TrailHistory.Num() - 1)), 1.f));
		}
	}
}

void UGurenUltimatePresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	Age += DeltaTime;
	const EGurenUltimateStage Stage = Skill->GetStage();
	UpdateCamera(DeltaTime);
	UpdateArm(DeltaTime);
	UpdateTargets(DeltaTime);
	if (!Effects.IsEmpty())
	{
		const USkeletalMeshComponent* Mesh = CastChecked<ACharacter>(GetOwner())->GetMesh();
		Effects[0]->SetWorldLocation(Arm && Arm->IsVisible() ? Arm->GetComponentTransform().TransformPosition(ArmPalmOffset) : Mesh->GetSocketLocation(TEXT("socket_palm_fx")));
	}
	if (Halo && Stage >= EGurenUltimateStage::Descent)
	{
		const float Deploy = Stage == EGurenUltimateStage::Descent ? FMath::SmoothStep(0.1f, 0.9f, Skill->GetStageTime() / Skill->GetStageDuration(Stage)) : 1.f;
		Halo->SetVisibility(true);
		Halo->SetWorldLocation(GetOwner()->GetActorTransform().TransformPosition(HaloOffset));
		Halo->SetWorldRotation(FRotationMatrix::MakeFromZY(GetOwner()->GetActorForwardVector(), FVector::UpVector).Rotator());
		Halo->SetWorldScale3D(FVector(HaloRadius / 50.f * GetOwner()->GetActorScale3D().GetAbsMax() * FMath::Lerp(0.7f, 1.f, Deploy) * (1.f + 0.012f * FMath::Sin(Age * 3.f))));
		HaloMID->SetScalarParameterValue(TEXT("Intensity"), Deploy * (Stage == EGurenUltimateStage::Recover ? FMath::Max(0.f, 1.f - Skill->GetStageTime() / Skill->GetStageDuration(Stage)) : 1.f));
	}
	const float NightAlpha = Stage == EGurenUltimateStage::Recover ? 1.f - FMath::Clamp(Skill->GetStageTime() / Skill->GetStageDuration(Stage), 0.f, 1.f)
		: FMath::Clamp(Age / 0.65f, 0.f, 1.f);
	for (const FSunState& State : Suns)
	{
		if (UDirectionalLightComponent* Light = State.Light.Get())
		{
			Light->SetIntensity(State.Intensity * FMath::Lerp(1.f, NightSunMultiplier, NightAlpha));
			Light->SetLightColor(FMath::Lerp(State.Color, FLinearColor(0.55f, 0.65f, 0.9f), NightAlpha));
		}
	}
	if (HeroLight)
	{
		const FVector Axis = Skill->GetAttackDirection();
		HeroLight->SetWorldLocation(GetOwner()->GetActorLocation() - Axis * 450.f + FVector::CrossProduct(FVector::UpVector, Axis) * 350.f + FVector(0, 0, 350));
		HeroLight->SetIntensity(HeroLightIntensity * NightAlpha);
	}
}

void UGurenUltimatePresentationComponent::MontageEnded(UAnimMontage* EndedMontage, bool bInterrupted)
{
	if (bInterrupted && Skill && Skill->IsUltimateActive())
	{
		Skill->Abort();
	}
}

void UGurenUltimatePresentationComponent::Cleanup()
{
	SetComponentTickEnabled(false);
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance(); Anim && Montage)
		{
			FOnMontageEnded Empty;
			Anim->Montage_SetEndDelegate(Empty, Montage);
			Anim->Montage_Stop(0.15f, Montage);
		}
		if (bArmHidden)
		{
			Character->GetMesh()->UnHideBoneByName(LaunchBone);
			bArmHidden = false;
		}
	}
	for (const FArrivalMaterialSnapshot& Snapshot : Materials)
	{
		if (UMeshComponent* Mesh = Snapshot.Mesh.Get())
		{
			for (int32 Index = 0; Index < Snapshot.Originals.Num(); ++Index)
			{
				Mesh->SetMaterial(Index, Snapshot.Originals[Index]);
			}
			Mesh->SetRelativeTransform(Snapshot.RelativeTransform);
			if (Snapshot.bBoundsFallback)
			{
				Mesh->DestroyComponent();
			}
		}
	}
	for (const FArrivalMaterialSnapshot& Snapshot : Materials)
	{
		for (UNiagaraComponent* Effect : { Snapshot.SurfaceEffect.Get(), Snapshot.PierceEffect.Get() })
		{
			if (IsValid(Effect))
			{
				Effect->DestroyComponent();
			}
		}
	}
	Materials.Reset();
	for (UNiagaraComponent* Effect : Effects)
	{
		if (IsValid(Effect))
		{
			Effect->DestroyComponent();
		}
	}
	Effects.Reset();
	for (UStaticMeshComponent* Component : Trails)
	{
		Component->DestroyComponent();
	}
	for (UNiagaraComponent* Component : TargetCharges)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	for (UStaticMeshComponent* Component : { Arm.Get(), Halo.Get(), NightSky.Get() })
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	Trails.Reset();
	TargetCharges.Reset();
	TrailHistory.Reset();
	Arm = Halo = NightSky = nullptr;
	HaloMID = nullptr;
	for (const FSunState& State : Suns)
	{
		if (UDirectionalLightComponent* Light = State.Light.Get())
		{
			Light->SetIntensity(State.Intensity);
			Light->SetLightColor(State.Color);
		}
	}
	Suns.Reset();
	if (HeroLight)
	{
		HeroLight->DestroyComponent();
		HeroLight = nullptr;
	}
	if (Player.IsValid() && OriginalViewTarget.IsValid())
	{
		Player->SetViewTarget(OriginalViewTarget.Get());
	}
	if (Camera)
	{
		Camera->Destroy();
		Camera = nullptr;
	}
	Player.Reset();
	OriginalViewTarget.Reset();
}

void UGurenUltimatePresentationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Skill)
	{
		Skill->Abort();
		Skill->OnStageChanged.RemoveDynamic(this, &UGurenUltimatePresentationComponent::StageChanged);
		Skill->OnTargetPierced.RemoveDynamic(this, &UGurenUltimatePresentationComponent::TargetPierced);
	}
	Cleanup();
	Super::EndPlay(Reason);
}
