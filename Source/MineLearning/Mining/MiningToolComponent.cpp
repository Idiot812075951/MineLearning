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

bool UMiningToolComponent::IsMining() const
{
    return bIsMining;
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

FVector UMiningToolComponent::GetMiningHitCenter() const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return FVector::ZeroVector;
    }

    const FVector Forward = Owner->GetActorForwardVector();
    USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();

    if (bUseOwnerMeshSocket && OwnerMesh && OwnerMesh->DoesSocketExist(HitSocketName))
    {
        return OwnerMesh->GetSocketLocation(HitSocketName);
    }

    return Owner->GetActorLocation()
        + Forward * StartForwardOffset
        + FVector(0.0f, 0.0f, StartHeightOffset);
}

bool UMiningToolComponent::PlayMiningMontage()
{
    USkeletalMeshComponent* Mesh = GetOwnerMesh();
    if (!Mesh || !MiningMontage)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[MiningTool] PlayMiningMontage=false: Mesh=%s Montage=%s Owner=%s"),
            *GetNameSafe(Mesh),
            *GetNameSafe(MiningMontage),
            *GetNameSafe(GetOwner())
        );
        return false;
    }

    UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MiningTool] PlayMiningMontage=false: no AnimInstance Mesh=%s Owner=%s"), *GetNameSafe(Mesh), *GetNameSafe(GetOwner()));
        return false;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EndMiningTimerHandle);
    }

    AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UMiningToolComponent::OnMiningMontageNotifyBegin);
    AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &UMiningToolComponent::OnMiningMontageNotifyBegin);

    const float Duration = AnimInstance->Montage_Play(MiningMontage);
    if (Duration <= 0.0f)
    {
        AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UMiningToolComponent::OnMiningMontageNotifyBegin);
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[MiningTool] PlayMiningMontage=false: Montage_Play Duration=%.2f Montage=%s Owner=%s"),
            Duration,
            *GetNameSafe(MiningMontage),
            *GetNameSafe(GetOwner())
        );
        return false;
    }

    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &UMiningToolComponent::OnMiningMontageEnded);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, MiningMontage);

    return true;
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

    // 非 Montage 挖矿模式：动画由 AnimBP 读取 bIsMining 驱动，C++ 直接造成伤害。
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

    // Montage 模式：StartMining 只播放动作，实际命中由 MiningHitNotifyName 对应的 Montage Notify 触发。
    if (!PlayMiningMontage())
    {
        EndMining();
        return false;
    }

    return true;
}

bool UMiningToolComponent::StartMiningTarget(AMineableOre* TargetOre)
{
    if (!IsValid(TargetOre) || TargetOre->IsDestroyed())
    {
        return false;
    }

    if (bIsMining)
    {
        return false;
    }

    ACharacter* Character = GetOwnerCharacter();

    bIsMining = true;
    ActiveMiningTarget = TargetOre;

    if (bLockMovementDuringMining && Character)
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            MoveComp->DisableMovement();
        }
    }

    if (!bUseMiningMontage)
    {
        const bool bHit = TryMineTarget(TargetOre);

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

    // Montage 模式：StartMiningTarget 只启动动作，真正伤害由 MiningHitNotifyName 对应的 Montage Notify 触发。
    if (!PlayMiningMontage())
    {
        EndMining();
        return false;
    }

    return true;
}

void UMiningToolComponent::EndMining()
{
    if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
    {
        if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
        {
            AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UMiningToolComponent::OnMiningMontageNotifyBegin);
        }
    }

    bIsMining = false;
    ActiveMiningTarget = nullptr;

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

void UMiningToolComponent::OnMiningMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
    if (NotifyName != MiningHitNotifyName)
    {
        return;
    }

    HandleMiningHitNotify();
}

void UMiningToolComponent::OnMiningMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage != MiningMontage)
    {
        return;
    }

    EndMining();
}

void UMiningToolComponent::HandleMiningHitNotify()
{
    if (IsValid(ActiveMiningTarget) && !ActiveMiningTarget->IsDestroyed())
    {
        TryMineTarget(ActiveMiningTarget);
        return;
    }

    TryMine();
}

bool UMiningToolComponent::TryMineTarget(AMineableOre* TargetOre)
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();

    if (!World || !Owner || !IsValid(TargetOre) || TargetOre->IsDestroyed())
    {
        return false;
    }

    const double Now = World->GetTimeSeconds();

    if (Now - LastMineTime < AttackInterval)
    {
        return false;
    }

    LastMineTime = Now;

    const FVector HitCenter = GetMiningHitCenter();

    if (bDrawDebug)
    {
        DrawDebugSphere(
            World,
            HitCenter,
            TraceRadius,
            16,
            FColor::Green,
            false,
            0.5f,
            0,
            2.0f
        );
    }

    FMiningHitRequest Request;
    Request.MiningPower = MiningPower;
    Request.ToolEfficiency = 1.0f;
    Request.InstigatorActor = Owner;
    Request.HitLocation = TargetOre->GetActorLocation();
    Request.HitNormal = (Owner->GetActorLocation() - TargetOre->GetActorLocation()).GetSafeNormal();

    return TargetOre->ApplyMiningHit(Request);
}

bool UMiningToolComponent::TryMine()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();

    if (!World || !Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MiningTool] TryMine=false: World=%s Owner=%s"), World ? TEXT("valid") : TEXT("null"), *GetNameSafe(Owner));
        return false;
    }

    const double Now = World->GetTimeSeconds();

    if (Now - LastMineTime < AttackInterval)
    {
        return false;
    }

    LastMineTime = Now;

    const FVector HitCenter = GetMiningHitCenter();
    const FVector Forward = Owner->GetActorForwardVector();

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
