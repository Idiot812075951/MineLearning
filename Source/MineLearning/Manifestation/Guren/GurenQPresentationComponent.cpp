#include "GurenQPresentationComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

UGurenQPresentationComponent::UGurenQPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	FRichCurve* IntensityCurve = RadiationIntensity.GetRichCurve();
	IntensityCurve->AddKey(0.f, 0.18f);
	IntensityCurve->AddKey(0.18f, 0.45f);
	IntensityCurve->AddKey(0.65f, 0.72f);
	IntensityCurve->AddKey(0.9f, 1.f);
	IntensityCurve->AddKey(1.f, 0.8f);
}

void UGurenQPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	Skill = GetOwner()->FindComponentByClass<UGurenQSkillComponent>();
	if (Skill)
	{
		Skill->OnStageChanged.AddDynamic(this, &UGurenQPresentationComponent::StageChanged);
		Skill->OnGrabContact.AddDynamic(this, &UGurenQPresentationComponent::GrabContact);
	}
	Camera = GetOwner()->FindComponentByClass<UCameraComponent>();
	CameraBoom = GetOwner()->FindComponentByClass<USpringArmComponent>();
}

float UGurenQPresentationComponent::GetRadiationDuration() const
{
	float Start = 0.f;
	float End = 0.f;
	if (GrabMontage)
	{
		for (const FAnimNotifyEvent& Notify : GrabMontage->Notifies)
		{
			if (const UAnimNotify_GurenQEvent* Event = Cast<UAnimNotify_GurenQEvent>(Notify.Notify))
			{
				if (Event->Event == TEXT("StartDissolve"))
				{
					Start = Notify.GetTriggerTime();
				}
				else if (Event->Event == TEXT("DissolveFinish"))
				{
					End = Notify.GetTriggerTime();
				}
			}
		}
	}
	return FMath::Max(0.01f, End - Start);
}

void UGurenQPresentationComponent::StageChanged(EGurenQStage Stage, AActor* Target)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim && Stage != EGurenQStage::Idle && Stage != EGurenQStage::Release)
	{
		Skill->Cancel();
		return;
	}
	if (Stage == EGurenQStage::Dash || Stage == EGurenQStage::Grab)
	{
		FramingScale = Skill->GetExecutionScale();
		LastRealTime = GetWorld()->GetRealTimeSeconds();
		SetComponentTickEnabled(true);
		if (FramingScale > 1.f && !bExecutionCameraActive)
		{
			BeginExecutionCamera();
		}
		float PlayRate = 1.f;
		if (Stage == EGurenQStage::Dash && DashMontage && Target)
		{
			const float Distance = FVector::Dist2D(Character->GetActorLocation(), Target->GetActorLocation());
			const float Duration = FMath::Clamp(DashBaseDuration + Distance / FMath::Max(1.f, DashTravelSpeed), static_cast<float>(DashDurationRange.X), static_cast<float>(DashDurationRange.Y));
			PlayRate = DashMontage->GetPlayLength() / Duration;
			NextAfterimageAt = LastRealTime + AfterimageDelay;
		}
		// Remove the old completion callback before the next montage interrupts it.
		FOnMontageEnded EmptyDelegate;
		if (PlayingMontage)
		{
			Anim->Montage_SetEndDelegate(EmptyDelegate, PlayingMontage);
		}
		PlayingMontage = Stage == EGurenQStage::Dash ? DashMontage : GrabMontage;
		if (!PlayingMontage || Anim->Montage_Play(PlayingMontage, PlayRate) <= 0.f)
		{
			Skill->Cancel();
			return;
		}
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UGurenQPresentationComponent::MontageEnded);
		Anim->Montage_SetEndDelegate(EndDelegate, PlayingMontage);
	}
	else if (Stage == EGurenQStage::Radiation)
	{
		ShakeStartedAt = GetWorld()->GetRealTimeSeconds();
		ShakeAmplitude = ImpactStrength;
		ShakeDuration = DissolveShakeDuration;
		RestoreTargetMaterials();
		TInlineComponentArray<UMeshComponent*> Meshes(Target);
		for (UMeshComponent* Mesh : Meshes)
		{
			FGurenQMaterialSnapshot& Snapshot = MaterialSnapshots.AddDefaulted_GetRef();
			Snapshot.Mesh = Mesh;
			Snapshot.bVisible = Mesh->IsVisible();
			Snapshot.bHiddenInGame = Mesh->bHiddenInGame;
			for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
			{
				Snapshot.Materials.Add(Mesh->GetMaterial(Index));
			}
		}
		RadiationChanged(true, Target, Skill->GetGripLocation());
	}
	else if (Stage == EGurenQStage::Release || Stage == EGurenQStage::Idle)
	{
		if (Stage == EGurenQStage::Release && IsValid(PalmRadiation) && RadiationHeatTail > 0.f)
		{
			RadiationTailEndsAt = GetWorld()->GetTimeSeconds() + RadiationHeatTail;
			UpdatePalmRadiation();
		}
		else if (RadiationTailEndsAt <= 0.0)
		{
			StopPalmRadiation();
		}
		RestoreTimeDilation();
		ReturnExecutionCamera();
		if (Stage == EGurenQStage::Idle)
		{
			ShakeDuration = 0.f;
			ClearAfterimages();
		}
		RadiationChanged(false, Target, FVector::ZeroVector);
		RestoreTargetMaterials();
		if (Stage == EGurenQStage::Idle && PlayingMontage && Anim)
		{
			FOnMontageEnded EmptyDelegate;
			Anim->Montage_SetEndDelegate(EmptyDelegate, PlayingMontage);
			Anim->Montage_Stop(0.15f, PlayingMontage);
			PlayingMontage = nullptr;
		}
	}
}

void UGurenQPresentationComponent::RestoreTargetMaterials()
{
	for (const FGurenQMaterialSnapshot& Snapshot : MaterialSnapshots)
	{
		if (UMeshComponent* Mesh = Snapshot.Mesh.Get())
		{
			for (int32 Index = 0; Index < Snapshot.Materials.Num(); ++Index)
			{
				Mesh->SetMaterial(Index, Snapshot.Materials[Index]);
			}
			Mesh->SetVisibility(Snapshot.bVisible);
			Mesh->SetHiddenInGame(Snapshot.bHiddenInGame);
		}
	}
	MaterialSnapshots.Reset();
}

void UGurenQPresentationComponent::GrabContact()
{
	StartPalmRadiation();
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}
	BeginExecutionCamera();
	// The demo is single-player. Bound the global hit emphasis in real time so
	// slowdown never lengthens its own restoration delay.
	RestoreTimeDilation();
	if (ContactSlowMotionDuration > 0.f)
	{
		PreviousTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
		UGameplayStatics::SetGlobalTimeDilation(this, PreviousTimeDilation * ContactTimeScale);
		SlowMotionUntil = GetWorld()->GetRealTimeSeconds() + ContactSlowMotionDuration;
	}
}

void UGurenQPresentationComponent::StartPalmRadiation()
{
	if (RadiationTailEndsAt > 0.0)
	{
		StopPalmRadiation();
	}
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (IsValid(PalmRadiation) || !PalmRadiationSystem || !Character || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh || PalmRadiationSocket.IsNone() || !Mesh->DoesSocketExist(PalmRadiationSocket))
	{
		return;
	}
	// Own this component for the whole contact window. An interrupted skill must
	// remove every particle immediately, even if its Niagara emitter loops forever.
	PalmRadiation = UNiagaraFunctionLibrary::SpawnSystemAttached(PalmRadiationSystem, Mesh,
		PalmRadiationSocket, PalmRadiationOffset.GetLocation(), PalmRadiationOffset.Rotator(),
		PalmRadiationOffset.GetScale3D(), EAttachLocation::KeepRelativeOffset, false,
		ENCPoolMethod::None, true, false);
	if (!PalmRadiation)
	{
		return;
	}
	RadiantMaterialIndex = Mesh->GetMaterialIndex(RadiantMaterialSlot);
	if (RadiantMaterialIndex != INDEX_NONE)
	{
		OriginalRadiantMaterial = Mesh->GetMaterial(RadiantMaterialIndex);
		RadiantMaterial = Mesh->CreateDynamicMaterialInstance(RadiantMaterialIndex);
	}
	if (RadiationLightIntensity > 0.f)
	{
		RadiationLight = NewObject<UPointLightComponent>(GetOwner());
		RadiationLight->SetupAttachment(PalmRadiation);
		RadiationLight->SetRelativeLocation(FVector(0.f, -8.f, 0.f));
		RadiationLight->SetCastShadows(false);
		RadiationLight->SetLightColor(FLinearColor(1.f, 0.035f, 0.055f));
		RadiationLight->SetIntensityUnits(ELightUnits::Lumens);
		RadiationLight->RegisterComponent();
	}
	SetComponentTickEnabled(true);
	UpdatePalmRadiation();
}

void UGurenQPresentationComponent::StopPalmRadiation()
{
	if (RadiationLight)
	{
		RadiationLight->DestroyComponent();
		RadiationLight = nullptr;
	}
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (OriginalRadiantMaterial && Character->GetMesh()->GetMaterial(RadiantMaterialIndex) == RadiantMaterial)
		{
			Character->GetMesh()->SetMaterial(RadiantMaterialIndex, OriginalRadiantMaterial);
		}
	}
	RadiantMaterial = nullptr;
	OriginalRadiantMaterial = nullptr;
	RadiantMaterialIndex = INDEX_NONE;
	RadiationTailEndsAt = 0.0;
	if (IsValid(PalmRadiation))
	{
		PalmRadiation->DestroyComponent();
	}
	PalmRadiation = nullptr;
}

void UGurenQPresentationComponent::UpdatePalmRadiation()
{
	if (!IsValid(PalmRadiation))
	{
		return;
	}
	float Intensity = 1.f;
	float Heat = 1.f;
	if (RadiationTailEndsAt > 0.0)
	{
		Heat = FMath::Clamp(static_cast<float>(RadiationTailEndsAt - GetWorld()->GetTimeSeconds()) / FMath::Max(0.001f, RadiationHeatTail), 0.f, 1.f);
		if (Heat <= 0.f)
		{
			StopPalmRadiation();
			return;
		}
		Intensity = 0.f;
		Heat *= 0.45f;
	}
	else if (GrabMontage)
	{
		const ACharacter* Character = CastChecked<ACharacter>(GetOwner());
		const UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
		float Start = 0.f;
		float End = GrabMontage->GetPlayLength();
		for (const FAnimNotifyEvent& Notify : GrabMontage->Notifies)
		{
			if (const UAnimNotify_GurenQEvent* Event = Cast<UAnimNotify_GurenQEvent>(Notify.Notify))
			{
				if (Event->Event == TEXT("GrabContact"))
				{
					Start = Notify.GetTriggerTime();
				}
				else if (Event->Event == TEXT("DissolveFinish"))
				{
					End = Notify.GetTriggerTime();
				}
			}
		}
		const float Time = Anim ? FMath::Max(0.f, Anim->Montage_GetPosition(GrabMontage) - Start) : 0.f;
		const float Progress = FMath::Clamp(Time / FMath::Max(0.01f, End - Start), 0.f, 1.f);
		const float Pulse = 1.f - RadiationPulseDepth * (0.5f - 0.5f * FMath::Cos(Time * RadiationPulseFrequency * UE_TWO_PI));
		Intensity = FMath::Clamp(RadiationIntensity.GetRichCurveConst()->Eval(Progress) * Pulse, 0.f, 1.f);
		Heat = Intensity;
	}
	PalmRadiation->SetVariableFloat(TEXT("User.Intensity"), Intensity);
	PalmRadiation->SetVariableFloat(TEXT("User.HeatIntensity"), Heat);
	if (RadiantMaterial)
	{
		RadiantMaterial->SetScalarParameterValue(TEXT("RadiantIntensity"), Intensity);
	}
	if (RadiationLight)
	{
		const float Scale = PalmRadiation->GetComponentScale().GetAbsMax();
		// A larger source needs proportionally more flux to keep equal surface illumination.
		RadiationLight->SetIntensity(RadiationLightIntensity * Intensity * Scale * Scale);
		RadiationLight->SetAttenuationRadius(RadiationLightRadius * Scale);
	}
}

void UGurenQPresentationComponent::BeginExecutionCamera()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	APlayerController* Controller = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!bUseExecutionCamera || !CameraBoom.IsValid() || !Controller || !Controller->IsLocalController())
	{
		return;
	}
	if (!bExecutionCameraActive)
	{
		CameraController = Controller;
		OriginalViewRotation = Controller->GetControlRotation();
		OriginalArmLength = CameraBoom->TargetArmLength;
		OriginalTargetOffset = CameraBoom->TargetOffset;
		Controller->SetIgnoreLookInput(true);
	}
	OrbitStartRotation = Controller->GetControlRotation();
	OrbitStartArmLength = CameraBoom->TargetArmLength;
	OrbitStartOffset = CameraBoom->TargetOffset;
	// The spring arm places the camera behind its viewing direction. Face back
	// toward Guren from the front, offset slightly to keep the victim off his face.
	ExecutionViewRotation = FRotator(ExecutionCameraPitch, Character->GetActorRotation().Yaw + 180.f + ExecutionCameraYawOffset, 0.f);
	OrbitStartedAt = GetWorld()->GetRealTimeSeconds();
	bExecutionCameraActive = true;
	bReturningCamera = false;
}

void UGurenQPresentationComponent::ReturnExecutionCamera(bool bImmediate)
{
	if (!bExecutionCameraActive)
	{
		return;
	}
	if (bImmediate)
	{
		if (CameraController.IsValid())
		{
			CameraController->SetControlRotation(OriginalViewRotation);
			CameraController->SetIgnoreLookInput(false);
		}
		if (CameraBoom.IsValid())
		{
			CameraBoom->TargetArmLength = OriginalArmLength;
			CameraBoom->TargetOffset = OriginalTargetOffset;
		}
		CameraController.Reset();
		bExecutionCameraActive = false;
		bReturningCamera = false;
	}
	else if (!bReturningCamera && CameraController.IsValid() && CameraBoom.IsValid())
	{
		OrbitStartRotation = CameraController->GetControlRotation();
		OrbitStartArmLength = CameraBoom->TargetArmLength;
		OrbitStartOffset = CameraBoom->TargetOffset;
		OrbitStartedAt = GetWorld()->GetRealTimeSeconds();
		bReturningCamera = true;
	}
}

void UGurenQPresentationComponent::UpdateExecutionCamera(double Now)
{
	if (!bExecutionCameraActive)
	{
		return;
	}
	if (!CameraController.IsValid() || !CameraBoom.IsValid())
	{
		ReturnExecutionCamera(true);
		return;
	}
	const float Duration = bReturningCamera ? ExecutionCameraBlendOut : ExecutionCameraBlendIn;
	const float LinearAlpha = FMath::Clamp(static_cast<float>(Now - OrbitStartedAt) / FMath::Max(Duration, 0.01f), 0.f, 1.f);
	const float Alpha = FMath::SmoothStep(0.f, 1.f, LinearAlpha);
	const FRotator GoalRotation = bReturningCamera ? OriginalViewRotation : ExecutionViewRotation;
	const float GoalArmLength = bReturningCamera ? OriginalArmLength : ExecutionCameraDistance * FramingScale;
	const ACharacter* Character = CastChecked<ACharacter>(GetOwner());
	const float Height = ExecutionCameraHeight * FramingScale + Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * (FramingScale - 1.f);
	const FVector GoalOffset = bReturningCamera ? OriginalTargetOffset : OriginalTargetOffset + FVector(0.f, 0.f, Height);
	// Interpolate shortest yaw/pitch arcs without introducing quaternion roll.
	const FRotator RotationDelta = (GoalRotation - OrbitStartRotation).GetNormalized();
	CameraController->SetControlRotation(OrbitStartRotation + RotationDelta * Alpha);
	CameraBoom->TargetArmLength = FMath::Lerp(OrbitStartArmLength, GoalArmLength, Alpha);
	CameraBoom->TargetOffset = FMath::Lerp(OrbitStartOffset, GoalOffset, Alpha);
	if (bReturningCamera && LinearAlpha >= 1.f)
	{
		ReturnExecutionCamera(true);
	}
}

void UGurenQPresentationComponent::RestoreTimeDilation()
{
	if (SlowMotionUntil > 0.0)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, PreviousTimeDilation);
		SlowMotionUntil = 0.0;
	}
}

void UGurenQPresentationComponent::SpawnAfterimage(double Now)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!AfterimageMaterial || !Character || Afterimages.Num() >= MaximumAfterimages)
	{
		return;
	}
	USkeletalMeshComponent* Source = Character->GetMesh();
	FGurenQAfterimage Image;
	Image.Mesh = NewObject<UPoseableMeshComponent>(GetOwner());
	Image.Mesh->SetSkinnedAssetAndUpdate(Source->GetSkeletalMeshAsset());
	Image.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Image.Mesh->SetCastShadow(false);
	Image.Mesh->SetComponentTickEnabled(false);
	Image.Mesh->SetWorldTransform(Source->GetComponentTransform());
	Image.Mesh->RegisterComponent();
	Image.Mesh->CopyPoseFromSkeletalComponent(Source);
	Image.Material = UMaterialInstanceDynamic::Create(AfterimageMaterial, this);
	Image.Material->SetScalarParameterValue(TEXT("Opacity"), AfterimageOpacity);
	for (int32 Index = 0; Index < Source->GetNumMaterials(); ++Index)
	{
		Image.Mesh->SetMaterial(Index, Image.Material);
	}
	Image.BornAt = Now;
	Afterimages.Add(Image);
}

void UGurenQPresentationComponent::ClearAfterimages()
{
	for (FGurenQAfterimage& Image : Afterimages)
	{
		Image.Mesh->DestroyComponent();
	}
	Afterimages.Reset();
}

void UGurenQPresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const double Now = GetWorld()->GetRealTimeSeconds();
	const float RealDelta = FMath::Clamp(static_cast<float>(Now - LastRealTime), 0.f, 0.25f);
	LastRealTime = Now;
	UpdateExecutionCamera(Now);
	UpdatePalmRadiation();
	if (SlowMotionUntil > 0.0 && Now >= SlowMotionUntil)
	{
		RestoreTimeDilation();
	}
	const EGurenQStage Stage = Skill ? Skill->GetStage() : EGurenQStage::Idle;
	for (int32 Index = Afterimages.Num() - 1; Index >= 0; --Index)
	{
		FGurenQAfterimage& Image = Afterimages[Index];
		const float Alpha = 1.f - static_cast<float>(Now - Image.BornAt) / FMath::Max(0.01f, AfterimageLifetime);
		if (Alpha <= 0.f)
		{
			Image.Mesh->DestroyComponent();
			Afterimages.RemoveAt(Index);
		}
		else
		{
			Image.Material->SetScalarParameterValue(TEXT("Opacity"), AfterimageOpacity * Alpha * Alpha);
		}
	}
	if (Stage == EGurenQStage::Dash && Now >= NextAfterimageAt && GetOwner()->GetVelocity().SizeSquared2D() > FMath::Square(AfterimageMinimumSpeed))
	{
		SpawnAfterimage(Now);
		NextAfterimageAt = Now + AfterimageInterval;
	}
	const float DesiredZoom = Stage == EGurenQStage::Radiation ? -CloseUpFOV
		: Stage == EGurenQStage::Grab ? -CloseUpFOV * 0.65f
		: Stage == EGurenQStage::Dash ? 4.f : 0.f;
	ZoomOffset = FMath::FInterpTo(ZoomOffset, DesiredZoom, RealDelta, 7.f);
	const float ShakeAge = static_cast<float>(Now - ShakeStartedAt);
	const float Envelope = ShakeDuration > 0.f ? FMath::Square(FMath::Max(0.f, 1.f - ShakeAge / ShakeDuration)) : 0.f;
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Camera.IsValid() && Character && Character->IsLocallyControlled())
	{
		const float Wave = FMath::Sin(ShakeAge * 150.f) * Envelope * ShakeAmplitude;
		const FRotator Rotation(Wave * 0.8f, Wave * 0.45f, Wave * 0.25f);
		// Additive camera offsets leave the player's wheel zoom and base FOV intact.
		const FVector Offset(0.f, Wave * 1.5f, Wave);
		Camera->ClearAdditiveOffset();
		Camera->AddAdditiveOffset(FTransform(Rotation, Offset), ZoomOffset);
	}
	if (Stage == EGurenQStage::Idle && !PalmRadiation && !bExecutionCameraActive && FMath::Abs(ZoomOffset) < 0.01f && Envelope <= 0.f && Afterimages.IsEmpty())
	{
		if (Camera.IsValid())
		{
			Camera->ClearAdditiveOffset();
		}
		ZoomOffset = 0.f;
		SetComponentTickEnabled(false);
	}
}

void UGurenQPresentationComponent::MontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != PlayingMontage || !Skill)
	{
		return;
	}
	PlayingMontage = nullptr;
	if (bInterrupted)
	{
		Skill->Cancel();
	}
	else
	{
		Skill->HandleAnimationEvent(Montage == DashMontage ? TEXT("DashArrival") : TEXT("SkillEnd"));
	}
}

void UGurenQPresentationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Skill)
	{
		Skill->Cancel();
		Skill->OnStageChanged.RemoveDynamic(this, &UGurenQPresentationComponent::StageChanged);
		Skill->OnGrabContact.RemoveDynamic(this, &UGurenQPresentationComponent::GrabContact);
	}
	RestoreTimeDilation();
	ReturnExecutionCamera(true);
	StopPalmRadiation();
	ClearAfterimages();
	if (Camera.IsValid())
	{
		Camera->ClearAdditiveOffset();
	}
	Super::EndPlay(Reason);
}

void UAnimNotify_GurenQEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (MeshComp && MeshComp->GetOwner())
	{
		if (UGurenQSkillComponent* Skill = MeshComp->GetOwner()->FindComponentByClass<UGurenQSkillComponent>())
		{
			Skill->HandleAnimationEvent(Event);
		}
	}
}
