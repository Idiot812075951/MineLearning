#include "CombatFeedbackComponent.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/UnitEffectComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"

UCombatFeedbackComponent::UCombatFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	RangeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MineLearning/GameplayRuntime/FX/M_AttackRange.M_AttackRange")));
	AuraMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MineLearning/GameplayRuntime/FX/M_StackAura.M_StackAura")));
}

void UCombatFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();
	Combat = GetOwner()->FindComponentByClass<UCombatComponent>();
	Effects = GetOwner()->FindComponentByClass<UUnitEffectComponent>();
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	Mesh = Character ? Character->GetMesh() : nullptr;
	if (Combat) { Combat->OnAttackOutOfRange.AddUniqueDynamic(this, &UCombatFeedbackComponent::ShowRange); }
	if (Effects)
	{
		Effects->OnEffectsChanged.AddUniqueDynamic(this, &UCombatFeedbackComponent::RefreshAura);
		RefreshAura();
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
	for (const FUnitEffectView& Effect : Effects->GetActiveEffectViews())
	{
		AuraIntensity = FMath::Max(AuraIntensity, Effect.AuraIntensity);
	}
	if (!Mesh) { return; }
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
	}
	else if (Aura && Mesh->GetOverlayMaterial() == Aura)
	{
		Mesh->SetOverlayMaterial(PreviousOverlay);
	}
}

void UCombatFeedbackComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Combat) { Combat->OnAttackOutOfRange.RemoveDynamic(this, &UCombatFeedbackComponent::ShowRange); }
	if (Effects) { Effects->OnEffectsChanged.RemoveDynamic(this, &UCombatFeedbackComponent::RefreshAura); }
	if (Mesh && Aura && Mesh->GetOverlayMaterial() == Aura) { Mesh->SetOverlayMaterial(PreviousOverlay); }
	HideRange();
	if (RangeRing) { RangeRing->DestroyComponent(); }
	Super::EndPlay(Reason);
}
