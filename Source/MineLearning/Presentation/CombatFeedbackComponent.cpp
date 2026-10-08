#include "CombatFeedbackComponent.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/UnitEffectComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"

UCombatFeedbackComponent::UCombatFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SteamMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MineLearning/GameplayRuntime/FX/M_OverheatSteam.M_OverheatSteam")));
	ChargeReadySound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/MineLearning/Characters/Guren/Ultimate/Audio/SW_Ult_Launch.SW_Ult_Launch")));
	RangeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MineLearning/GameplayRuntime/FX/M_AttackRange.M_AttackRange")));
	AuraMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MineLearning/GameplayRuntime/FX/M_StackAura.M_StackAura")));
	WorkRingMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MineLearning/GameplayRuntime/FX/M_WorkRing.M_WorkRing")));
	WorkMarkerMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MineLearning/GameplayRuntime/FX/M_WorkMarker.M_WorkMarker")));
}

void UCombatFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();
	Combat = GetOwner()->FindComponentByClass<UCombatComponent>();
	Effects = GetOwner()->FindComponentByClass<UUnitEffectComponent>();
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	Mesh = Character ? Character->GetMesh() : nullptr;
	if (Mesh) { OriginalMeshScale = Mesh->GetRelativeScale3D(); }
	if (Combat)
	{
		ObservedAttackRange = Combat->GetAttackRange();
		Combat->OnAttackOutOfRange.AddUniqueDynamic(this, &UCombatFeedbackComponent::ShowRange);
		Combat->OnAttributesChanged.AddUniqueDynamic(this, &UCombatFeedbackComponent::AttributesChanged);
	}
	if (Effects)
	{
		Effects->OnEffectsChanged.AddUniqueDynamic(this, &UCombatFeedbackComponent::RefreshAura);
		RefreshAura();
	}
}

void UCombatFeedbackComponent::AttributesChanged()
{
	const float Range = Combat->GetAttackRange();
	if (!FMath::IsNearlyEqual(Range, ObservedAttackRange))
	{
		ObservedAttackRange = Range;
		ShowRange();
	}
}

void UCombatFeedbackComponent::ShowRange()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Combat || !Pawn || !Pawn->IsPlayerControlled() || !Pawn->IsLocallyControlled()) { return; }
	if (!RangeRing)
	{
		UMaterialInterface* Material = RangeMaterial.LoadSynchronous();
		UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		if (!Material || !Plane) { return; }
		RangeRing = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("AttackRangeRing"));
		RangeRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RangeRing->SetCanEverAffectNavigation(false);
		RangeRing->SetCastShadow(false);
		RangeRing->SetStaticMesh(Plane);
		RangeRing->SetMaterial(0, Material);
		GetOwner()->AddInstanceComponent(RangeRing);
		RangeRing->RegisterComponent();
	}
	RangeEndTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.01f, RangeDisplaySeconds);
	RangeRing->SetVisibility(true);
	UpdateRange();
	GetWorld()->GetTimerManager().SetTimer(RangeTimer, this, &UCombatFeedbackComponent::UpdateRange, 0.025f, true);
}

void UCombatFeedbackComponent::UpdateRange()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled() || !Pawn->IsPlayerControlled() || GetWorld()->GetTimeSeconds() >= RangeEndTime)
	{
		HideRange();
		return;
	}
	const FVector Origin = Pawn->GetActorLocation();
	FHitResult Ground;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(AttackRangeGround), false, Pawn);
	const bool bGround = GetWorld()->LineTraceSingleByChannel(Ground, Origin, Origin - FVector(0, 0, 1500), ECC_Visibility, Query);
	RangeRing->SetWorldLocation((bGround ? Ground.ImpactPoint : Origin - FVector(0, 0, 90)) + FVector(0, 0, 3));
	// Ring material puts its outer edge at UV radius 0.5; engine plane is 100 cm wide.
	RangeRing->SetWorldScale3D(FVector(Combat->GetAttackRange() * 0.02f));
}

void UCombatFeedbackComponent::HideRange()
{
	if (RangeRing) { RangeRing->SetVisibility(false); }
	GetWorld()->GetTimerManager().ClearTimer(RangeTimer);
}

bool UCombatFeedbackComponent::IsRangeVisible() const
{
	return RangeRing && RangeRing->IsVisible();
}

void UCombatFeedbackComponent::RefreshAura()
{
	AuraIntensity = 0.f;
	float ScaleBonus = 0.f;
	float RingIntensity = 0.f;
	bool bMarker = false;
	bool bToolGlow = false;
	FLinearColor ToolColor = FLinearColor::White;
	bool bSteam = false;
	TMap<FName, TWeakObjectPtr<USoundBase>> Sounds;
	FLinearColor Color(1.f, 0.12f, 0.025f);
	FLinearColor RingColor = FLinearColor::White;
	for (const FUnitEffectView& Effect : Effects->GetActiveEffectViews())
	{
		if (Effect.AuraIntensity > AuraIntensity) { AuraIntensity = Effect.AuraIntensity; Color = Effect.AuraColor; }
		ScaleBonus += Effect.VisualScaleBonus;
		if (Effect.GroundRingIntensity > RingIntensity) { RingIntensity = Effect.GroundRingIntensity; RingColor = Effect.AuraColor; }
		bMarker |= Effect.bOverheadMarker;
		bToolGlow |= Effect.bToolGlow;
		if (Effect.bToolGlow) { ToolColor = Effect.AuraColor; }
		bSteam |= Effect.bSteam;
		if (Effect.ActivationSound)
		{
			Sounds.Add(Effect.Source, Effect.ActivationSound);
			const TWeakObjectPtr<USoundBase>* Previous = ActiveSounds.Find(Effect.Source);
			if (!Previous || Previous->Get() != Effect.ActivationSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, Effect.ActivationSound, GetOwner()->GetActorLocation(), 0.4f);
			}
		}
	}
	ActiveSounds = MoveTemp(Sounds);
	if (!Mesh) { return; }
	Mesh->SetRelativeScale3D(OriginalMeshScale * FMath::Clamp(1.f + ScaleBonus, 0.25f, 3.f));
	RefreshWorkIndicators(RingIntensity, bMarker, RingColor);
	RefreshToolGlow(bToolGlow, ToolColor);
	RefreshSteam(bSteam);
	if (AuraIntensity > 0.f)
	{
		if (!Aura)
		{
			UMaterialInterface* Material = AuraMaterial.LoadSynchronous();
			if (!Material) { return; }
			Aura = UMaterialInstanceDynamic::Create(Material, this);
		}
		if (Mesh->GetOverlayMaterial() != Aura)
		{
			PreviousOverlay = Mesh->GetOverlayMaterial();
			Mesh->SetOverlayMaterial(Aura);
		}
		Aura->SetScalarParameterValue(TEXT("Intensity"), AuraIntensity);
		Aura->SetVectorParameterValue(TEXT("AuraColor"), Color);
	}
	else if (Aura && Mesh->GetOverlayMaterial() == Aura)
	{
		Mesh->SetOverlayMaterial(PreviousOverlay);
	}
}

void UCombatFeedbackComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Combat) { Combat->OnAttackOutOfRange.RemoveDynamic(this, &UCombatFeedbackComponent::ShowRange); }
	if (Combat) { Combat->OnAttributesChanged.RemoveDynamic(this, &UCombatFeedbackComponent::AttributesChanged); }
	RefreshToolGlow(false, FLinearColor::White);
	if (Effects) { Effects->OnEffectsChanged.RemoveDynamic(this, &UCombatFeedbackComponent::RefreshAura); }
	if (Mesh) { Mesh->SetRelativeScale3D(OriginalMeshScale); }
	if (Mesh && Aura && Mesh->GetOverlayMaterial() == Aura) { Mesh->SetOverlayMaterial(PreviousOverlay); }
	HideRange();
	if (RangeRing) { RangeRing->DestroyComponent(); }
	if (WorkRing) { WorkRing->DestroyComponent(); }
	if (WorkMarker) { WorkMarker->DestroyComponent(); }
	if (ToolLight) { ToolLight->DestroyComponent(); }
	for (UStaticMeshComponent* Plane : SteamPlanes) { if (Plane) { Plane->DestroyComponent(); } }
	Super::EndPlay(Reason);
}

void UCombatFeedbackComponent::RefreshWorkIndicators(float Intensity, bool bMarker, const FLinearColor& Color)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const float HalfHeight = Character ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 60.f;
	if (Intensity > 0.f && !WorkRing)
	{
		if (UMaterialInterface* Material = WorkRingMaterial.LoadSynchronous())
		{
			WorkRing = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("WorkPowerRing"));
			WorkRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			WorkRing->SetCanEverAffectNavigation(false);
			WorkRing->SetCastShadow(false);
			WorkRing->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
			WorkRingMID = UMaterialInstanceDynamic::Create(Material, this);
			WorkRing->SetMaterial(0, WorkRingMID);
			GetOwner()->AddInstanceComponent(WorkRing);
			WorkRing->RegisterComponent();
			WorkRing->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			WorkRing->SetRelativeLocation(FVector(0.f, 0.f, -HalfHeight + 4.f));
			WorkRing->SetRelativeScale3D(FVector(1.6f));
		}
	}
	if (WorkRing)
	{
		WorkRing->SetVisibility(Intensity > 0.f);
		WorkRingMID->SetScalarParameterValue(TEXT("Intensity"), Intensity);
		WorkRingMID->SetVectorParameterValue(TEXT("Tint"), Color);
	}
	if (bMarker && !WorkMarker)
	{
		if (UMaterialInterface* Material = WorkMarkerMaterial.LoadSynchronous())
		{
			WorkMarker = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("FocusWorkMarker"));
			WorkMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			WorkMarker->SetCanEverAffectNavigation(false);
			WorkMarker->SetCastShadow(false);
			WorkMarker->SetVisibility(false);
			WorkMarker->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			WorkMarkerMID = UMaterialInstanceDynamic::Create(Material, this);
			WorkMarker->SetMaterial(0, WorkMarkerMID);
			GetOwner()->AddInstanceComponent(WorkMarker);
			WorkMarker->RegisterComponent();
			WorkMarker->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			WorkMarker->SetRelativeLocation(FVector(0.f, 0.f, HalfHeight + 45.f));
			WorkMarker->SetRelativeRotation(FRotator(45.f, 45.f, 0.f));
			WorkMarker->SetRelativeScale3D(FVector(0.2f));
		}
	}
	if (WorkMarker)
	{
		if (bMarker && !WorkMarker->IsVisible()) { WorkMarkerMID->SetScalarParameterValue(TEXT("PulseStart"), GetWorld()->GetTimeSeconds()); }
		WorkMarker->SetVisibility(bMarker);
		WorkMarkerMID->SetVectorParameterValue(TEXT("Tint"), Color);
	}
}

void UCombatFeedbackComponent::RefreshToolGlow(bool bEnabled, const FLinearColor& Color)
{
	if (bEnabled && !ToolMaterial && Mesh)
	{
		const int32 Slot = Mesh->GetMaterialIndex(ToolMaterialSlot);
		if (Slot != INDEX_NONE) { ToolMaterial = Mesh->CreateDynamicMaterialInstance(Slot); }
	}
	if (ToolMaterial)
	{
		ToolMaterial->SetScalarParameterValue(TEXT("ChargeIntensity"), bEnabled ? 1.f : 0.f);
		ToolMaterial->SetVectorParameterValue(TEXT("ChargeColor"), Color * 4.f);
	}
	if (bEnabled && !ToolLight && Mesh && Mesh->DoesSocketExist(ToolGlowSocket))
	{
		ToolLight = NewObject<UPointLightComponent>(GetOwner(), TEXT("ChargedToolLight"));
		ToolLight->SetCastShadows(false);
		ToolLight->SetAttenuationRadius(130.f);
		ToolLight->SetIntensity(2500.f);
		GetOwner()->AddInstanceComponent(ToolLight);
		ToolLight->RegisterComponent();
		ToolLight->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ToolGlowSocket);
	}
	if (ToolLight) { ToolLight->SetVisibility(bEnabled); ToolLight->SetLightColor(Color); }
	if (bEnabled && !bToolWasGlowing)
	{
		if (USoundBase* Sound = ChargeReadySound.LoadSynchronous()) { UGameplayStatics::PlaySoundAtLocation(this, Sound, GetOwner()->GetActorLocation(), 0.25f, 1.5f); }
	}
	bToolWasGlowing = bEnabled;
}

void UCombatFeedbackComponent::RefreshSteam(bool bEnabled)
{
	if (bEnabled && SteamPlanes.IsEmpty())
	{
		UMaterialInterface* Material = SteamMaterial.LoadSynchronous();
		UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		if (!Material || !PlaneMesh) { return; }
		for (int32 Index = 0; Index < 2; ++Index)
		{
			UStaticMeshComponent* Plane = NewObject<UStaticMeshComponent>(GetOwner());
			Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Plane->SetCanEverAffectNavigation(false);
			Plane->SetCastShadow(false);
			Plane->SetStaticMesh(PlaneMesh);
			Plane->SetMaterial(0, Material);
			GetOwner()->AddInstanceComponent(Plane);
			Plane->RegisterComponent();
			Plane->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			Plane->SetRelativeRotation(FRotator(90.f, Index * 90.f, 0.f));
			Plane->SetRelativeLocation(FVector(0.f, 0.f, 65.f));
			Plane->SetRelativeScale3D(FVector(1.3f, 2.f, 1.f));
			SteamPlanes.Add(Plane);
		}
	}
	for (UStaticMeshComponent* Plane : SteamPlanes) { Plane->SetVisibility(bEnabled); }
}
