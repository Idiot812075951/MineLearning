#include "MiningToolComponent.h"

#include "MineableOre.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

UMiningToolComponent::UMiningToolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UMiningToolComponent::TryMine()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();

    if (!World || !Owner)
    {
        return false;
    }

    const double Now = World->GetTimeSeconds();

    if (Now - LastMineTime < AttackInterval)
    {
        return false;
    }

    LastMineTime = Now;

    FVector HitCenter;
    const FVector Forward = Owner->GetActorForwardVector();

    USkeletalMeshComponent* OwnerMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();

    if (bUseOwnerMeshSocket && OwnerMesh && OwnerMesh->DoesSocketExist(HitSocketName))
    {
        HitCenter = OwnerMesh->GetSocketLocation(HitSocketName);
    }
    else
    {
        const FVector OwnerLocation = Owner->GetActorLocation();

        HitCenter =
            OwnerLocation
            + Forward * StartForwardOffset
            + FVector(0.0f, 0.0f, StartHeightOffset);
    }

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);
    Params.bTraceComplex = false;

    TArray<FOverlapResult> Overlaps;

    const bool bHasOverlap = World->OverlapMultiByChannel(
        Overlaps,
        HitCenter,
        FQuat::Identity,
        TraceChannel,
        FCollisionShape::MakeSphere(TraceRadius),
        Params
    );

    if (bDrawDebug)
    {
        DrawDebugSphere(
            World,
            HitCenter,
            TraceRadius,
            16,
            bHasOverlap ? FColor::Green : FColor::Red,
            false,
            0.5f,
            0,
            2.0f
        );
    }

    if (!bHasOverlap)
    {
        return false;
    }

    AMineableOre* BestOre = nullptr;
    UPrimitiveComponent* BestComp = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (const FOverlapResult& Result : Overlaps)
    {
        AMineableOre* Ore = Cast<AMineableOre>(Result.GetActor());
        if (!Ore || Ore->IsDestroyed())
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(Ore->GetActorLocation(), HitCenter);

        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestOre = Ore;
            BestComp = Result.GetComponent();
        }
    }

    if (!BestOre)
    {
        return false;
    }

    FVector ActualHitLocation = HitCenter;
    FVector ActualHitNormal = -Forward;

    if (BestComp)
    {
        FVector ClosestPoint;
        const float Distance = BestComp->GetClosestPointOnCollision(HitCenter, ClosestPoint);

        if (Distance >= 0.0f)
        {
            ActualHitLocation = ClosestPoint;
            ActualHitNormal = (HitCenter - ClosestPoint).GetSafeNormal();

            if (ActualHitNormal.IsNearlyZero())
            {
                ActualHitNormal = -Forward;
            }
        }
    }

    FMiningHitRequest Request;
    Request.MiningPower = MiningPower;
    Request.ToolEfficiency = 1.0f;
    Request.InstigatorActor = Owner;
    Request.HitLocation = ActualHitLocation;
    Request.HitNormal = ActualHitNormal;

    return BestOre->ApplyMiningHit(Request);
}