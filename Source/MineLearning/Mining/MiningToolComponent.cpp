#include "MiningToolComponent.h"

#include "MineableOre.h"
#include "MiningTypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UMiningToolComponent::UMiningToolComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMiningToolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FinishMining(true, false);
    Super::EndPlay(EndPlayReason);
}

bool UMiningToolComponent::IsMining() const
{
    return bIsMining;
}

void UMiningToolComponent::SetMiningHitSocketName(FName NewSocketName)
{
    HitSocketName = NewSocketName;
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

    if (bUseOwnerMeshSocket && OwnerMesh)
    {
        if (OwnerMesh->DoesSocketExist(HitSocketName))
        {
            return OwnerMesh->GetSocketLocation(HitSocketName);
        }

        static const FName DrillTipSocketName(TEXT("S_DrillTip"));
        if (OwnerMesh->DoesSocketExist(DrillTipSocketName))
        {
            return OwnerMesh->GetSocketLocation(DrillTipSocketName);
        }
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

    const float Duration = AnimInstance->Montage_Play(MiningMontage, GetClampedAttackSpeed());
    if (Duration <= 0.0f)
    {
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

bool UMiningToolComponent::ResolveMiningLoopRange()
{
    MiningLoopStartTime = 0.0f;
    MiningLoopEndTime = 0.0f;

    if (!MiningMontage)
    {
        return false;
    }

    for (const FSlotAnimationTrack& SlotTrack : MiningMontage->SlotAnimTracks)
    {
        bool bFoundStart = false;
        bool bFoundLoop = false;
        float LoopStartTime = 0.0f;
        float LoopEndTime = 0.0f;

        for (const FAnimSegment& Segment : SlotTrack.AnimTrack.AnimSegments)
        {
            const UAnimSequenceBase* Animation = Segment.GetAnimReference();
            if (!Animation || Segment.LoopingCount != 1)
            {
                continue;
            }

            const FString AnimationName = Animation->GetName();
            if (!bFoundLoop && AnimationName.Contains(TEXT("Start"), ESearchCase::IgnoreCase))
            {
                bFoundStart = true;
                continue;
            }

            if (bFoundStart && !bFoundLoop && AnimationName.Contains(TEXT("Loop"), ESearchCase::IgnoreCase))
            {
                LoopStartTime = Segment.StartPos;
                LoopEndTime = Segment.GetEndPos();
                bFoundLoop = true;
                continue;
            }

            if (bFoundLoop && AnimationName.Contains(TEXT("End"), ESearchCase::IgnoreCase))
            {
                MiningLoopStartTime = LoopStartTime;
                MiningLoopEndTime = LoopEndTime;
                return MiningLoopEndTime > MiningLoopStartTime + KINDA_SMALL_NUMBER;
            }
        }
    }

    return false;
}

void UMiningToolComponent::ScheduleNextMiningHit()
{
    if (!bIsMining || NextMiningHitIndex >= ActiveMiningHitCount)
    {
        return;
    }

    if (!IsValid(ActiveMiningTarget) || ActiveMiningTarget->IsDestroyed())
    {
        StopScheduledMiningHits();
        return;
    }

    UWorld* World = GetWorld();
    USkeletalMeshComponent* Mesh = GetOwnerMesh();
    UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
    if (!World || !AnimInstance || !AnimInstance->Montage_IsPlaying(MiningMontage))
    {
        StopScheduledMiningHits();
        return;
    }

    const float EffectivePlayRate = FMath::Abs(AnimInstance->Montage_GetEffectivePlayRate(MiningMontage));
    if (EffectivePlayRate <= KINDA_SMALL_NUMBER)
    {
        StopScheduledMiningHits();
        return;
    }

    const float TargetPosition = GetMiningHitMontageTime(NextMiningHitIndex);
    const float CurrentPosition = AnimInstance->Montage_GetPosition(MiningMontage);
    const float Delay = FMath::Max(
        (TargetPosition - CurrentPosition) / EffectivePlayRate,
        KINDA_SMALL_NUMBER
    );

    World->GetTimerManager().SetTimer(
        MiningHitTimerHandle,
        this,
        &UMiningToolComponent::HandleScheduledMiningHit,
        Delay,
        false
    );
}

void UMiningToolComponent::HandleScheduledMiningHit()
{
    if (!bIsMining || NextMiningHitIndex >= ActiveMiningHitCount)
    {
        return;
    }

    if (!IsValid(ActiveMiningTarget) || ActiveMiningTarget->IsDestroyed())
    {
        StopScheduledMiningHits();
        return;
    }

    const int32 HitNumber = NextMiningHitIndex + 1;
    ++NextMiningHitIndex;
    const bool bHit = ApplyMiningHitToTarget(ActiveMiningTarget);
    UE_LOG(
        LogTemp,
        Log,
        TEXT("[MiningTool] CycleHit=%d/%d Result=%s Owner=%s Target=%s"),
        HitNumber,
        ActiveMiningHitCount,
        bHit ? TEXT("Confirmed") : TEXT("Rejected"),
        *GetNameSafe(GetOwner()),
        *GetNameSafe(ActiveMiningTarget)
    );

    if (!bHit || !IsValid(ActiveMiningTarget) || ActiveMiningTarget->IsDestroyed())
    {
        StopScheduledMiningHits();
        return;
    }

    ScheduleNextMiningHit();
}

void UMiningToolComponent::StopScheduledMiningHits()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MiningHitTimerHandle);
    }
}

int32 UMiningToolComponent::GetClampedMiningHitCount() const
{
    return FMath::Clamp(MiningHitCount, MinMiningHitCount, MaxMiningHitCount);
}

float UMiningToolComponent::GetClampedAttackSpeed() const
{
    return FMath::Clamp(AttackSpeed, MinAttackSpeed, MaxAttackSpeed);
}

float UMiningToolComponent::GetMiningHitMontageTime(int32 HitIndex) const
{
    const float LoopLength = MiningLoopEndTime - MiningLoopStartTime;
    const float HitAlpha = (static_cast<float>(HitIndex) + 0.5f) / static_cast<float>(ActiveMiningHitCount);
    return MiningLoopStartTime + LoopLength * HitAlpha;
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

    if (!ResolveMiningLoopRange())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[MiningTool] StartMiningTarget=false: montage must contain one-shot Start -> Loop -> End segments Montage=%s Owner=%s"),
            *GetNameSafe(MiningMontage),
            *GetNameSafe(GetOwner())
        );
        return false;
    }

    bIsMining = true;
    ActiveMiningTarget = TargetOre;
    ActiveMiningHitCount = GetClampedMiningHitCount();
    NextMiningHitIndex = 0;
    LockOwnerMovementAndRotation();

    if (!PlayMiningMontage())
    {
        FinishMining(true, false);
        return false;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[MiningTool] CycleStart Hits=%d Speed=%.2f Loop=%.3f-%.3f Owner=%s Target=%s"),
        ActiveMiningHitCount,
        GetClampedAttackSpeed(),
        MiningLoopStartTime,
        MiningLoopEndTime,
        *GetNameSafe(GetOwner()),
        *GetNameSafe(ActiveMiningTarget)
    );
    ScheduleNextMiningHit();

    return true;
}

void UMiningToolComponent::FinishMining(bool bInterrupted, bool bBroadcastCompletion)
{
    const bool bWasMining = bIsMining;

    StopScheduledMiningHits();

    bIsMining = false;
    ActiveMiningHitCount = 0;
    NextMiningHitIndex = 0;
    MiningLoopStartTime = 0.0f;
    MiningLoopEndTime = 0.0f;
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

void UMiningToolComponent::OnMiningMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage != MiningMontage)
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[MiningTool] CycleEnd Interrupted=%s Owner=%s"),
        bInterrupted ? TEXT("true") : TEXT("false"),
        *GetNameSafe(GetOwner())
    );
    FinishMining(bInterrupted, true);
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
    Request.bPlayTargetHitFeedback = false;

    const FVector HitCenter = GetMiningHitCenter();
    Request.HitLocation = TargetOre->GetActorLocation();
    Request.HitNormal = (HitCenter - Request.HitLocation).GetSafeNormal();

    if (UStaticMeshComponent* OreMesh = TargetOre->GetOreMesh())
    {
        FVector ClosestPoint;
        if (OreMesh->GetClosestPointOnCollision(HitCenter, ClosestPoint) >= 0.0f)
        {
            Request.HitLocation = ClosestPoint;
            Request.HitNormal = (HitCenter - ClosestPoint).GetSafeNormal();
        }
    }

    if (Request.HitNormal.IsNearlyZero())
    {
        Request.HitNormal = (Owner->GetActorLocation() - TargetOre->GetActorLocation()).GetSafeNormal();
    }

    if (Request.HitNormal.IsNearlyZero())
    {
        Request.HitNormal = FVector::UpVector;
    }

    if (!TargetOre->ApplyMiningHit(Request))
    {
        return false;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[MiningTool] MiningHitConfirmed Location=%s Normal=%s Owner=%s"),
        *Request.HitLocation.ToCompactString(),
        *Request.HitNormal.ToCompactString(),
        *GetNameSafe(Owner)
    );
    OnMiningHitConfirmed.Broadcast(Request.HitLocation, Request.HitNormal);
    return true;
}
