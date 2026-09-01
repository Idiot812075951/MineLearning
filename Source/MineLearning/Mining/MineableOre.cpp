#include "MineableOre.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "OreDefinitionDataAsset.h"
#include "ResourcePickup.h"
#include "ResourceHitFeedbackComponent.h"
#include "OreVisualComponent.h"

AMineableOre::AMineableOre()
{
    PrimaryActorTick.bCanEverTick = false;

    OreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OreMesh"));
    SetRootComponent(OreMesh);

    HitFeedbackComponent = CreateDefaultSubobject<UResourceHitFeedbackComponent>(TEXT("HitFeedbackComponent"));
    OreVisualComponent = CreateDefaultSubobject<UOreVisualComponent>(TEXT("OreVisualComponent"));

    OreMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    OreMesh->SetCollisionObjectType(ECC_WorldStatic);
    OreMesh->SetCollisionResponseToAllChannels(ECR_Block);
    // Mineable resources are interaction targets, not permanent terrain. Keeping them
    // out of the navigation export prevents a resource from cutting an impassable ring
    // around the exact place the companion must stand to mine it.
    OreMesh->SetCanEverAffectNavigation(false);
}

void AMineableOre::BeginPlay()
{
    Super::BeginPlay();

    InitializeStatsFromDefinition();

    if (BaseMaterial)
    {
        DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        OreMesh->SetMaterial(0, DynamicMaterial);
    }

    ApplyDamageVisual();

}

void AMineableOre::SetOreDefinition(UOreDefinitionDataAsset* InOreDefinition)
{
    OreDefinition = InOreDefinition;

    if (HasActorBegunPlay())
    {
        InitializeStatsFromDefinition();
		ApplyDamageVisual();
    }
}

void AMineableOre::InitializeStatsFromDefinition()
{
    if (OreDefinition)
    {
        MaxHP = FMath::Max(OreDefinition->MaxHealth, 1.0f);
    }

    CurrentHP = MaxHP;
    bHasDepleted = false;
	CurrentMiningStageIndex = 0;
	SettledBreakThresholdIndices.Reset();
    OnOreHealthChanged.Broadcast(CurrentHP, MaxHP);
}

bool AMineableOre::ApplyMiningHit(const FMiningHitRequest& Request)
{
    if (IsDestroyed())
    {
        return false;
    }

	const float PreviousHealth = CurrentHP;
	const float ActualDamage = (Request.MiningPower * Request.ToolEfficiency) / FMath::Max(Hardness, 0.01f);

	CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.0f, MaxHP);
	const bool bStageBreak = ProcessStageBreaks(PreviousHealth, Request.HitLocation);
	UE_LOG(LogTemp, Log, TEXT("OreVisual: Route=%s HP=%.1f/%.1f NormalHitFX=%s"),
		bStageBreak ? TEXT("StageBreak") : TEXT("Hit"), CurrentHP, MaxHP,
		bStageBreak ? TEXT("Skipped") : TEXT("Play"));
	OnOreHealthChanged.Broadcast(CurrentHP, MaxHP);

	if (!bStageBreak && Request.bPlayTargetHitFeedback && HitFeedbackComponent)
	{
		HitFeedbackComponent->PlayHitFeedback(Request.HitLocation, Request.HitNormal, ActualDamage);
    }

	if (!UsesStageBreakResourceDrops())
	{
		SpawnDropsForTrigger(EOreDropTrigger::OnMiningHit, Request.HitLocation);
	}
    ApplyDamageVisual();

    if (CurrentHP <= 0.0f)
    {
        HandleDepleted();
    }

    return true;
}

bool AMineableOre::ApplyFatalMiningHit(const FMiningHitRequest& Request)
{
    if (IsDestroyed())
    {
        return false;
    }

	const float PreviousHealth = CurrentHP;
    CurrentHP = 0.0f;
	ProcessStageBreaks(PreviousHealth, Request.HitLocation);
    OnOreHealthChanged.Broadcast(CurrentHP, MaxHP);
    if (HitFeedbackComponent)
    {
		if (Request.bPlayTargetHitFeedback)
		{
			HitFeedbackComponent->PlayHitFeedback(Request.HitLocation, Request.HitNormal, MaxHP);
		}
        HitFeedbackComponent->PlayDestroyedFeedback(GetActorLocation(), Request.HitNormal);
    }
	if (!UsesStageBreakResourceDrops())
	{
		SpawnDropsForTrigger(EOreDropTrigger::OnMiningHit, Request.HitLocation);
	}
    ApplyDamageVisual();
    HandleDepleted();
    return true;
}

void AMineableOre::ApplyDamageVisual()
{
    const float DamageRatio = 1.0f - CurrentHP / FMath::Max(MaxHP, 0.01f);

    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(TEXT("DamageAmount"), DamageRatio);
    }

}

void AMineableOre::SpawnDropsForTrigger(EOreDropTrigger Trigger, const FVector& DropLocation)
{
    if (!OreDefinition || !OreDefinition->ResourcePickupClass)
    {
        return;
    }

    for (int32 RuleIndex = 0; RuleIndex < OreDefinition->DropRules.Num(); ++RuleIndex)
    {
        const FOreDropRule& DropRule = OreDefinition->DropRules[RuleIndex];
        if (DropRule.Trigger != Trigger)
        {
            continue;
        }

        const float DropChance = FMath::Clamp(DropRule.DropChance, 0.0f, 1.0f);
        const float DropRoll = FMath::FRand();
        if (DropChance <= 0.0f || DropRoll > DropChance)
        {
            continue;
        }

        const int32 MinAmount = FMath::Max(DropRule.MinAmount, 0);
        const int32 MaxAmount = FMath::Max(DropRule.MaxAmount, MinAmount);
        const int32 DropAmount = FMath::RandRange(MinAmount, MaxAmount);
        if (DropAmount > 0)
        {
            SpawnResourceDropDirect(DropRule.ResourceType, DropAmount, DropLocation);
        }
    }
}

bool AMineableOre::ProcessStageBreaks(float PreviousHealth, const FVector& DropLocation)
{
	if (!UsesStageBreakResourceDrops())
	{
		return false;
	}

	const float PreviousRatio = FMath::Clamp(PreviousHealth / FMath::Max(MaxHP, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float CurrentRatio = FMath::Clamp(CurrentHP / FMath::Max(MaxHP, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	TArray<int32> NewlyCrossedIndices;
	const TArray<float>& BreakThresholds = GetBreakThresholds();
	for (int32 Index = 0; Index < BreakThresholds.Num(); ++Index)
	{
		const float Threshold = FMath::Clamp(BreakThresholds[Index], 0.0f, 1.0f);
		if (!SettledBreakThresholdIndices.Contains(Index)
			&& PreviousRatio > Threshold + KINDA_SMALL_NUMBER
			&& CurrentRatio <= Threshold + KINDA_SMALL_NUMBER)
		{
			NewlyCrossedIndices.Add(Index);
		}
	}

	NewlyCrossedIndices.Sort([this](const int32 Left, const int32 Right)
	{
		const TArray<float>& BreakThresholds = GetBreakThresholds();
		return BreakThresholds[Left] > BreakThresholds[Right];
	});

	for (const int32 Index : NewlyCrossedIndices)
	{
		SettledBreakThresholdIndices.Add(Index);
		++CurrentMiningStageIndex;
		SpawnStageResourceDrops(DropLocation);
	}

	return !NewlyCrossedIndices.IsEmpty();
}

bool AMineableOre::UsesStageBreakResourceDrops() const
{
	return OreDefinition
		&& OreDefinition->ResourcePickupClass
		&& !GetBreakThresholds().IsEmpty();
}

const TArray<float>& AMineableOre::GetBreakThresholds() const
{
	static const TArray<float> DefaultBreakThresholds = { 0.8f, 0.6f, 0.4f, 0.2f, 0.0f };
	return OreDefinition && !OreDefinition->BreakThresholds.IsEmpty()
		? OreDefinition->BreakThresholds
		: DefaultBreakThresholds;
}

EResourceType AMineableOre::GetStageDropResourceType() const
{
	if (OreDefinition)
	{
		for (const FOreDropRule& DropRule : OreDefinition->DropRules)
		{
			if (DropRule.Trigger == EOreDropTrigger::OnMiningHit)
			{
				return DropRule.ResourceType;
			}
		}
	}

	return EResourceType::Stone;
}

void AMineableOre::SpawnStageResourceDrops(const FVector& DropLocation)
{
	if (!OreDefinition)
	{
		return;
	}

	// Future technology modifiers belong here, after the fixed stage payout is read.
	const int32 FinalAmount = FMath::Max(OreDefinition->ResourcePerStage, 0);
	const EResourceType ResourceType = GetStageDropResourceType();
	for (int32 PickupIndex = 0; PickupIndex < FinalAmount; ++PickupIndex)
	{
		SpawnResourceDropDirect(ResourceType, 1, DropLocation);
	}
}

void AMineableOre::HandleDepleted()
{
    if (bHasDepleted)
    {
        return;
    }

    bHasDepleted = true;
    SpawnDropsForTrigger(EOreDropTrigger::OnDepleted, GetActorLocation());
    OnOreDepleted.Broadcast(this);

    DestroyOre();
}

void AMineableOre::DestroyOre()
{
    Destroy();
}

bool AMineableOre::SpawnResourceDropDirect(EResourceType Type, int32 Amount, const FVector& DropLocation)
{
    if (!OreDefinition || !OreDefinition->ResourcePickupClass || Amount <= 0)
    {
        return false;
    }

    const FVector SpawnLocation = DropLocation + FVector(
        FMath::RandRange(-40.0f, 40.0f),
        FMath::RandRange(-40.0f, 40.0f),
        80.0f
    );

    AResourcePickup* Pickup = GetWorld()->SpawnActor<AResourcePickup>(
        OreDefinition->ResourcePickupClass,
        SpawnLocation,
        FRotator::ZeroRotator
    );

    if (Pickup)
    {
		Pickup->InitializeResource(
			Type,
			Amount,
			OreDefinition->DropMeshes
		);
        return true;
    }

    return false;
}
