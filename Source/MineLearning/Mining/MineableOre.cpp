#include "MineableOre.h"
#include "Components/StaticMeshComponent.h"
#include "OreDefinitionDataAsset.h"
#include "ResourcePickup.h"

AMineableOre::AMineableOre()
{
    PrimaryActorTick.bCanEverTick = false;

    OreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OreMesh"));
    SetRootComponent(OreMesh);

    OreMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    OreMesh->SetCollisionObjectType(ECC_WorldStatic);
    OreMesh->SetCollisionResponseToAllChannels(ECR_Block);
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
        UpdateDamageStage();
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
    OnOreHealthChanged.Broadcast(CurrentHP, MaxHP);
}

bool AMineableOre::ApplyMiningHit(const FMiningHitRequest& Request)
{
    if (IsDestroyed())
    {
        return false;
    }

    const float ActualDamage = (Request.MiningPower * Request.ToolEfficiency) / FMath::Max(Hardness, 0.01f);

    CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.0f, MaxHP);
    OnOreHealthChanged.Broadcast(CurrentHP, MaxHP);

    SpawnDropsForTrigger(EOreDropTrigger::OnMiningHit, Request.HitLocation);
    UpdateDamageStage();
    ApplyDamageVisual();

    if (CurrentHP <= 0.0f)
    {
        HandleDepleted();
    }

    return true;
}

void AMineableOre::UpdateDamageStage()
{
    const float Ratio = CurrentHP / MaxHP;

    EMineableDamageStage NewStage = EMineableDamageStage::Full;

    if (Ratio <= 0.0f)
    {
        NewStage = EMineableDamageStage::Destroyed;
    }
    else if (Ratio <= 0.25f)
    {
        NewStage = EMineableDamageStage::HeavyDamage;
    }
    else if (Ratio <= 0.5f)
    {
        NewStage = EMineableDamageStage::MediumDamage;
    }
    else if (Ratio <= 0.75f)
    {
        NewStage = EMineableDamageStage::LightDamage;
    }

    CurrentStage = NewStage;
}

void AMineableOre::ApplyDamageVisual()
{
    const float DamageRatio = 1.0f - CurrentHP / FMath::Max(MaxHP, 0.01f);

    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(TEXT("DamageAmount"), DamageRatio);
    }

    // 占位表现：越受损越缩小一点点，别太夸张
    const float Scale = FMath::Lerp(1.0f, 0.5f, DamageRatio);
    OreMesh->SetWorldScale3D(FVector(Scale));
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
        Pickup->InitializeResource(Type, Amount);
        return true;
    }

    return false;
}
