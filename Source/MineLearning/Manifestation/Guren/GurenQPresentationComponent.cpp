#include "GurenQPresentationComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"

UGurenQPresentationComponent::UGurenQPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
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

void UGurenQPresentationComponent::StageChanged(EGurenQStage Stage, AActor* Target)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim)
	{
		Skill->Cancel();
		return;
	}
	if (Stage == EGurenQStage::Dash || Stage == EGurenQStage::Grab)
	{
		LastRealTime = GetWorld()->GetRealTimeSeconds();
		SetComponentTickEnabled(true);
		float PlayRate = 1.f;
		if (Stage == EGurenQStage::Dash && DashMontage && Target)
		{
			const float Distance = FVector::Dist2D(Character->GetActorLocation(), Target->GetActorLocation());
			const float Duration = FMath::Clamp(0.48f + Distance / 3500.f, 0.57f, 1.05f);
			PlayRate = DashMontage->GetPlayLength() / Duration;
			NextAfterimageAt = LastRealTime + 0.12;
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
		RadiatingMesh = Target ? Target->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
		OriginalMaterials.Reset();
		if (RadiatingMesh.IsValid())
		{
			for (int32 Index = 0; Index < RadiatingMesh->GetNumMaterials(); ++Index)
			{
				OriginalMaterials.Add(RadiatingMesh->GetMaterial(Index));
			}
		}
		RadiationChanged(true, Target, Character->GetMesh()->GetSocketLocation(TEXT("Q_GrabHead")));
	}
	else if (Stage == EGurenQStage::Release || Stage == EGurenQStage::Idle)
	{
		RestoreTimeDilation();
		ReturnExecutionCamera();
		if (Stage == EGurenQStage::Idle)
		{
			ShakeDuration = 0.f;
			ClearAfterimages();
		}
		RadiationChanged(false, Target, FVector::ZeroVector);
		// The shared effect's editor Reset creates fresh preview MIDs. A cancelled
		// gameplay effect must return the exact materials it replaced instead.
		if (RadiatingMesh.IsValid())
		{
			for (int32 Index = 0; Index < OriginalMaterials.Num(); ++Index)
			{
				RadiatingMesh->SetMaterial(Index, OriginalMaterials[Index]);
			}
		}
		RadiatingMesh.Reset();
		OriginalMaterials.Reset();
		if (Stage == EGurenQStage::Idle && PlayingMontage)
		{
			FOnMontageEnded EmptyDelegate;
			Anim->Montage_SetEndDelegate(EmptyDelegate, PlayingMontage);
			Anim->Montage_Stop(0.15f, PlayingMontage);
			PlayingMontage = nullptr;
		}
	}
}

void UGurenQPresentationComponent::GrabContact()
{
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
	const float GoalArmLength = bReturningCamera ? OriginalArmLength : ExecutionCameraDistance;
	const FVector GoalOffset = bReturningCamera ? OriginalTargetOffset : OriginalTargetOffset + FVector(0.f, 0.f, ExecutionCameraHeight);
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
	if (!AfterimageMaterial || !Character || Afterimages.Num() >= 4)
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
	Image.Material->SetScalarParameterValue(TEXT("Opacity"), 0.24f);
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
	if (SlowMotionUntil > 0.0 && Now >= SlowMotionUntil)
	{
		RestoreTimeDilation();
	}
	const EGurenQStage Stage = Skill ? Skill->GetStage() : EGurenQStage::Idle;
	for (int32 Index = Afterimages.Num() - 1; Index >= 0; --Index)
	{
		FGurenQAfterimage& Image = Afterimages[Index];
		const float Alpha = 1.f - static_cast<float>(Now - Image.BornAt) / 0.24f;
		if (Alpha <= 0.f)
		{
			Image.Mesh->DestroyComponent();
			Afterimages.RemoveAt(Index);
		}
		else
		{
			Image.Material->SetScalarParameterValue(TEXT("Opacity"), 0.24f * Alpha * Alpha);
		}
	}
	if (Stage == EGurenQStage::Dash && Now >= NextAfterimageAt && GetOwner()->GetVelocity().SizeSquared2D() > FMath::Square(300.f))
	{
		SpawnAfterimage(Now);
		NextAfterimageAt = Now + 0.065;
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
	if (Stage == EGurenQStage::Idle && !bExecutionCameraActive && FMath::Abs(ZoomOffset) < 0.01f && Envelope <= 0.f && Afterimages.IsEmpty())
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
