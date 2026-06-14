#include "MiningToolComponent.h"

#include "MineableOre.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UMiningToolComponent::UMiningToolComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

ACharacter* UMiningToolComponent::GetOwnerCharacter() const
{
    return Cast<ACharacter>(GetOwner());
}

USkeletalMeshComponent* UMiningToolComponent::GetOwnerMesh() const
{
    if (const ACharacter* Character = GetOwnerCharacter())
    {
        return Character->GetMesh();
    }

    return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

bool UMiningToolComponent::StartMining()
{
    if (bIsMining)
    {
        return false;
    }

    ACharacter* Character = GetOwnerCharacter();

    bIsMining = true;

    if (bLockMovementDuringMining && Character)
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            MoveComp->DisableMovement();
        }
    }

    // Static Mesh 机器人：不走蒙太奇，直接检测挖矿。
    if (!bUseMiningMontage)
    {
        const bool bHit = TryMine();

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(EndMiningTimerHandle);
            World->GetTimerManager().SetTimer(
                EndMiningTimerHandle,
                this,
                &UMiningToolComponent::EndMining,
                NonMontageMiningDuration,
                false
            );
        }
        else
        {
            EndMining();
        }

        return bHit;
    }

    // 人形角色：走蒙太奇 + AnimNotify。
    USkeletalMeshComponent* Mesh = GetOwnerMesh();

    if (!Mesh || !MiningMontage)
    {
        EndMining();
        return false;
    }

    UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
    if (!AnimInstance)
    {
        EndMining();
        return false;
    }

    const float Duration = AnimInstance->Montage_Play(MiningMontage);

    if (Duration <= 0.0f)
    {
        EndMining();
        return false;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EndMiningTimerHandle);
        World->GetTimerManager().SetTimer(
            EndMiningTimerHandle,
            this,
            &UMiningToolComponent::EndMining,
            Duration,
            false
        );
    }

    return true;
}

void UMiningToolComponent::EndMining()
{
    bIsMining = false;

    if (bLockMovementDuringMining)
    {
        if (ACharacter* Character = GetOwnerCharacter())
        {
            if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
            {
                MoveComp->SetMovementMode(MOVE_Walking);
            }
        }
    }
}

void UMiningToolComponent::HandleMiningHitNotify()
{
    TryMine();
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

    FVector HitCenter = Owner->GetActorLocation();
    const FVector Forward = Owner->GetActorForwardVector();

    USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();

    if (bUseOwnerMeshSocket && OwnerMesh && OwnerMesh->DoesSocketExist(HitSocketName))
    {
        HitCenter = OwnerMesh->GetSocketLocation(HitSocketName);
    }
    else
    {
        HitCenter =
            Owner->GetActorLocation()
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
        AActor* HitActor = Result.GetActor();
        if (!HitActor)
        {
            continue;
        }

        AMineableOre* Ore = Cast<AMineableOre>(HitActor);
        if (!Ore || Ore->IsDestroyed())
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(
            Ore->GetActorLocation(),
            HitCenter
        );

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
        const float Distance = BestComp->GetClosestPointOnCollision(
            HitCenter,
            ClosestPoint
        );

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
