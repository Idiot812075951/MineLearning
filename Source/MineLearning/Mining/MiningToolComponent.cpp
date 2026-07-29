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

void UMiningToolComponent::CancelMining()
{
    if (!bIsMining && !bMovementAndRotationLocked)
    {
        return;
    }

    if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
    {
        if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
        {
            if (MiningMontage && AnimInstance->Montage_IsPlaying(MiningMontage))
            {
                AnimInstance->Montage_Stop(0.0f, MiningMontage);
            }
        }
    }

    if (bIsMining || bMovementAndRotationLocked)
    {
        FinishMining(true, true);
    }
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

    bIsMining = true;
    bStartedWithTarget = false;
    LockOwnerMovementAndRotation();

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
        FinishMining(true, false);
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

    bIsMining = true;
    bStartedWithTarget = true;
    ActiveMiningTarget = TargetOre;
    LockOwnerMovementAndRotation();

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
        FinishMining(true, false);
        return false;
    }

    return true;
}

void UMiningToolComponent::EndMining()
{
    FinishMining(false, true);
}

void UMiningToolComponent::FinishMining(bool bInterrupted, bool bBroadcastCompletion)
{
    const bool bWasMining = bIsMining;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EndMiningTimerHandle);
    }

    if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
    {
        if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
        {
            AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UMiningToolComponent::OnMiningMontageNotifyBegin);
        }
    }

    bIsMining = false;
    bStartedWithTarget = false;
    ActiveMiningTarget = nullptr;
    RestoreOwnerMovementAndRotation();

    if (bBroadcastCompletion && bWasMining)
    {
        OnMiningFinished.Broadcast(bInterrupted);
    }
}

void UMiningToolComponent::LockOwnerMovementAndRotation()
{
    if (!bLockMovementDuringMining || bMovementAndRotationLocked)
    {
        return;
    }

    ACharacter* Character = GetOwnerCharacter();
    if (!Character)
    {
        return;
    }

    bPreviousUseControllerRotationYaw = Character->bUseControllerRotationYaw;
    Character->bUseControllerRotationYaw = false;

    UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    bHadMovementComponent = MoveComp != nullptr;
    if (MoveComp)
    {
        PreviousMovementMode = MoveComp->MovementMode;
        PreviousCustomMovementMode = MoveComp->CustomMovementMode;
        bPreviousOrientRotationToMovement = MoveComp->bOrientRotationToMovement;
        bPreviousUseControllerDesiredRotation = MoveComp->bUseControllerDesiredRotation;

        MoveComp->StopMovementImmediately();
        MoveComp->bOrientRotationToMovement = false;
        MoveComp->bUseControllerDesiredRotation = false;
        MoveComp->DisableMovement();
    }

    bMovementAndRotationLocked = true;
}

void UMiningToolComponent::RestoreOwnerMovementAndRotation()
{
    if (!bMovementAndRotationLocked)
    {
        return;
    }

    if (ACharacter* Character = GetOwnerCharacter())
    {
        Character->bUseControllerRotationYaw = bPreviousUseControllerRotationYaw;

        if (bHadMovementComponent)
        {
            if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
            {
                MoveComp->bOrientRotationToMovement = bPreviousOrientRotationToMovement;
                MoveComp->bUseControllerDesiredRotation = bPreviousUseControllerDesiredRotation;
                MoveComp->SetMovementMode(PreviousMovementMode, PreviousCustomMovementMode);
            }
        }
    }

    bMovementAndRotationLocked = false;
    bHadMovementComponent = false;
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

    FinishMining(bInterrupted, true);
}

void UMiningToolComponent::HandleMiningHitNotify()
{
    if (bStartedWithTarget)
    {
        if (IsValid(ActiveMiningTarget) && !ActiveMiningTarget->IsDestroyed())
        {
            // The move request is the range gate. Once the action has started,
            // each matching Montage Notify directly hits its assigned target.
            ApplyMiningHitToTarget(ActiveMiningTarget);
        }

        return;
    }

    TryMine();
}

bool UMiningToolComponent::TryMineTarget(AMineableOre* TargetOre)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const double Now = World->GetTimeSeconds();

    if (Now - LastMineTime < AttackInterval)
    {
        return false;
    }

    LastMineTime = Now;
    return ApplyMiningHitToTarget(TargetOre);
}

bool UMiningToolComponent::ApplyMiningHitToTarget(AMineableOre* TargetOre)
{
    AActor* Owner = GetOwner();
    if (!Owner || !IsValid(TargetOre) || TargetOre->IsDestroyed())
    {
        return false;
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
