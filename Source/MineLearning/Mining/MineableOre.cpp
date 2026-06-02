#include "MineableOre.h"
#include "Components/StaticMeshComponent.h"
#include "ResourcePickup.h"
#include "Kismet/KismetMathLibrary.h"

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

    CurrentHP = MaxHP;

    if (BaseMaterial)
    {
        DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        OreMesh->SetMaterial(0, DynamicMaterial);
    }

    ApplyDamageVisual();
}

bool AMineableOre::ApplyMiningHit(const FMiningHitRequest& Request)
{
    if (IsDestroyed())
    {
        return false;
    }

    const float ActualDamage = (Request.MiningPower * Request.ToolEfficiency) / FMath::Max(Hardness, 0.01f);

    CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.0f, MaxHP);

    TryDropResource(Request.HitLocation);
    UpdateDamageStage();
    ApplyDamageVisual();

    if (CurrentHP <= 0.0f)
    {
        DestroyOre();
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

void AMineableOre::TryDropResource(const FVector& DropLocation)
{
    if (RemainingResourceAmount <= 0)
    {
        return;
    }

    if (FMath::FRand() > DropConfig.DropChance)
    {
        return;
    }

    const int32 DropAmount = FMath::Min(DropConfig.AmountPerDrop, RemainingResourceAmount);
    RemainingResourceAmount -= DropAmount;

    SpawnResourceDropDirect(DropConfig.ResourceType, DropAmount, DropLocation);
}

void AMineableOre::DestroyOre()
{
    if (RemainingResourceAmount > 0)
    {
        SpawnResourceDropDirect(
            DropConfig.ResourceType,
            RemainingResourceAmount,
            GetActorLocation()
        );

        RemainingResourceAmount = 0;
    }

    Destroy();
}

void AMineableOre::SpawnResourceDropDirect(EResourceType Type, int32 Amount, const FVector& DropLocation)
{
    if (!ResourcePickupClass || Amount <= 0)
    {
        return;
    }

    const FVector SpawnLocation = DropLocation + FVector(
        FMath::RandRange(-40.0f, 40.0f),
        FMath::RandRange(-40.0f, 40.0f),
        80.0f
    );

    AResourcePickup* Pickup = GetWorld()->SpawnActor<AResourcePickup>(
        ResourcePickupClass,
        SpawnLocation,
        FRotator::ZeroRotator
    );

    if (Pickup)
    {
        Pickup->InitializeResource(Type, Amount);
    }
}