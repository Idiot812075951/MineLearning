#include "GurenQSkillComponent.h"
#include "MineLearning/Combat/CombatDamageSubsystem.h"
#include "MineLearning/Combat/HealthComponent.h"

#include "MineLearning/Interaction/GrabbableComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "MotionWarpingComponent.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogGurenQ, Log, All);

UGurenQSkillComponent::UGurenQSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGurenQSkillComponent::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(SelectionTimer, this, &UGurenQSkillComponent::RefreshTargets, SelectionInterval, true);
	RefreshTargets();
}

AActor* UGurenQSkillComponent::GetTarget() const
{
	return Target.IsValid() ? Target->GetOwner() : nullptr;
}

TArray<UGrabbableComponent*> UGurenQSkillComponent::GetCandidates() const
{
	TArray<UGrabbableComponent*> Result;
	for (const TWeakObjectPtr<UGrabbableComponent>& Candidate : Candidates)
	{
		if (Candidate.IsValid())
		{
			Result.Add(Candidate.Get());
		}
	}
	return Result;
}

FVector UGurenQSkillComponent::GetGripLocation() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	return Character ? Character->GetMesh()->GetSocketLocation(GripSocket) : GetOwner()->GetActorLocation();
}

float UGurenQSkillComponent::CalculateScale(const UGrabbableComponent* Candidate) const
{
	const ACharacter* Character = CastChecked<ACharacter>(GetOwner());
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float FeetZ = Character->GetActorLocation().Z - HalfHeight;
	const float HeightScale = (Candidate->GetComponentLocation().Z - FeetZ) / FMath::Max(1.f, ContactOffset.Z + HalfHeight);
	return FMath::Max3(1.f, Candidate->GetGripDiameter() / FMath::Max(1.f, ClawDiameter), HeightScale);
}

bool UGurenQSkillComponent::CanSelect(UGrabbableComponent* Candidate) const
{
	if (!IsValid(Candidate) || !Candidate->CanGrab(GetOwner()) || !CanExecuteTarget(Candidate->GetOwner()) || CalculateScale(Candidate) > MaximumScale
		|| FVector::DistSquared(GetOwner()->GetActorLocation(), Candidate->GetComponentLocation()) > FMath::Square(SelectionRange))
	{
		return false;
	}
	if (bRequireLineOfSight)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GurenQSelection), false, GetOwner());
		if (GetWorld()->LineTraceSingleByChannel(Hit, GetOwner()->GetActorLocation(), Candidate->GetComponentLocation(), ECC_Visibility, Params)
			&& Hit.GetActor() != Candidate->GetOwner())
		{
			return false;
		}
	}
	return true;
}

void UGurenQSkillComponent::RefreshTargets()
{
	TArray<TWeakObjectPtr<UGrabbableComponent>> NewCandidates;
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!IsQActive() && Character && Character->IsLocallyControlled() && Character->GetCharacterMovement()->IsMovingOnGround())
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			UGrabbableComponent* Candidate = It->FindComponentByClass<UGrabbableComponent>();
			if (CanSelect(Candidate))
			{
				NewCandidates.Add(Candidate);
			}
		}
		// Stable cycle order while objects move. Nearest is only the initial/fallback selection.
		NewCandidates.Sort([](const TWeakObjectPtr<UGrabbableComponent>& A, const TWeakObjectPtr<UGrabbableComponent>& B)
		{
			return A->GetOwner()->GetPathName() < B->GetOwner()->GetPathName();
		});
	}
	UGrabbableComponent* NewSelection = SelectedTarget.Get();
	if (!NewCandidates.Contains(NewSelection))
	{
		NewSelection = nullptr;
		float NearestDistance = TNumericLimits<float>::Max();
		for (const TWeakObjectPtr<UGrabbableComponent>& Candidate : NewCandidates)
		{
			const float Distance = FVector::DistSquared(GetOwner()->GetActorLocation(), Candidate->GetComponentLocation());
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				NewSelection = Candidate.Get();
			}
		}
	}
	if (Candidates != NewCandidates || SelectedTarget.Get() != NewSelection)
	{
		Candidates = MoveTemp(NewCandidates);
		SelectedTarget = NewSelection;
		OnTargetsChanged.Broadcast();
	}
}

void UGurenQSkillComponent::CycleTarget()
{
	if (IsQActive())
	{
		return;
	}
	RefreshTargets();
	if (Candidates.Num() > 1)
	{
		SelectedTarget = Candidates[(Candidates.IndexOfByKey(SelectedTarget) + 1) % Candidates.Num()];
		OnTargetsChanged.Broadcast();
	}
}

void UGurenQSkillComponent::SetStage(EGurenQStage NewStage)
{
	Stage = NewStage;
	UE_LOG(LogGurenQ, Verbose, TEXT("Q stage=%s target=%s"), *UEnum::GetValueAsString(Stage), *GetNameSafe(GetTarget()));
	OnStageChanged.Broadcast(Stage, GetTarget());
}

void UGurenQSkillComponent::TryCast()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (IsQActive() || bStarting || bFinishing || !Character || !Character->HasAuthority() || !Character->GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}
	TGuardValue<bool> StartingGuard(bStarting, true);
	RefreshTargets();
	UGrabbableComponent* Candidate = SelectedTarget.Get();
	if (!CanSelect(Candidate) || !Character->GetMesh()->DoesSocketExist(GripSocket))
	{
		return;
	}
	if (!Candidate->Reserve(Character))
	{
		RefreshTargets();
		return;
	}
	Target = Candidate;
	OriginalMeshTransform = Character->GetMesh()->GetRelativeTransform();
	ExecutionScale = CalculateScale(Candidate);
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FTransform ScaledMesh = OriginalMeshTransform;
	ScaledMesh.SetScale3D(OriginalMeshTransform.GetScale3D() * ExecutionScale);
	ScaledMesh.SetLocation((OriginalMeshTransform.GetLocation() + FVector(0.f, 0.f, HalfHeight)) * ExecutionScale - FVector(0.f, 0.f, HalfHeight));
	Character->GetMesh()->SetRelativeTransform(ScaledMesh);
	FVector Direction = (Candidate->GetComponentLocation() - Character->GetActorLocation()).GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		Direction = Character->GetActorForwardVector();
	}
	const FQuat Facing = Direction.Rotation().Quaternion();
	FVector StandLocation = Candidate->GetComponentLocation() - Facing.RotateVector(ContactOffset * ExecutionScale);
	StandLocation.Z = Character->GetActorLocation().Z;
	StandTransform = FTransform(Facing, StandLocation);
	Character->StopJumping();
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->ConsumeMovementInputVector();
	bOriginalOrientToMovement = Character->GetCharacterMovement()->bOrientRotationToMovement;
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->SetActorRotation(Facing);
	LockedController = Character->GetController();
	if (LockedController.IsValid())
	{
		LockedController->SetIgnoreMoveInput(true);
	}
	Candidate->GetOwner()->OnDestroyed.AddDynamic(this, &UGurenQSkillComponent::TargetDestroyed);
	if (UMotionWarpingComponent* Warp = Character->FindComponentByClass<UMotionWarpingComponent>())
	{
		Warp->AddOrUpdateWarpTargetFromTransform(TEXT("Q_DashTarget"), StandTransform);
	}
	SetComponentTickEnabled(true);
	GetWorld()->GetTimerManager().SetTimer(Watchdog, this, &UGurenQSkillComponent::Cancel, ExecutionTimeout, false);
	const float Distance = FVector::Dist2D(Character->GetActorLocation(), StandLocation);
	SetStage(Distance > DirectGrabRange ? EGurenQStage::Dash : EGurenQStage::Grab);
	RefreshTargets();
}

void UGurenQSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Target.IsValid() && Stage != EGurenQStage::Release)
	{
		Cancel();
		return;
	}
	if (Stage == EGurenQStage::Grab && !bAttached)
	{
		GetOwner()->SetActorLocation(FMath::VInterpTo(GetOwner()->GetActorLocation(), StandTransform.GetLocation(), DeltaTime, ReachSpeed), true);
	}
}

void UGurenQSkillComponent::HandleAnimationEvent(FName Event)
{
	if (!IsQActive() || bFinishing)
	{
		return;
	}
	ACharacter* Character = CastChecked<ACharacter>(GetOwner());
	if (Event == TEXT("DashArrival") && Stage == EGurenQStage::Dash && !GetWorld()->GetTimerManager().TimerExists(DashArrivalTimer))
	{
		DashArrivalTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGurenQSkillComponent::FinishDash);
	}
	else if (Event == TEXT("GrabContact") && Stage == EGurenQStage::Grab && !bAttached)
	{
		// Low objects lift to the hand pose; horizontal contact must still be reachable.
		if (!Target.IsValid() || !CanExecuteTarget(Target->GetOwner()) || FVector::Dist2D(GetGripLocation(), Target->GetComponentLocation()) > ContactTolerance * ExecutionScale
			|| !Target->AttachToGrip(Character->GetMesh(), GripSocket))
		{
			Cancel();
			return;
		}
		bAttached = true;
		FCombatDamageRequest Request;
		Request.Source = GetOwner();
		Request.Target = GetTarget();
		Request.SkillId = TEXT("QGrab");
		Request.bNonLethal = true;
		Request.HitLocation = GetGripLocation();
		GetWorld()->GetSubsystem<UCombatDamageSubsystem>()->ApplyDamage(Request);
		OnGrabContact.Broadcast();
	}
	else if (Event == TEXT("StartDissolve") && Stage == EGurenQStage::Grab && bAttached)
	{
		FCombatDamageRequest Request;
		Request.Source = GetOwner();
		Request.Target = GetTarget();
		Request.SkillId = TEXT("QRadiation");
		Request.bNonLethal = true;
		Request.HitLocation = GetGripLocation();
		GetWorld()->GetSubsystem<UCombatDamageSubsystem>()->ApplyDamage(Request);
		SetStage(EGurenQStage::Radiation);
	}
	else if (Event == TEXT("DissolveFinish") && Stage == EGurenQStage::Radiation)
	{
		SetStage(EGurenQStage::Release);
		if (Target.IsValid())
		{
			Target->GetOwner()->OnDestroyed.RemoveDynamic(this, &UGurenQSkillComponent::TargetDestroyed);
			AActor* Victim = GetTarget();
			Target->Release(false);
			FCombatDamageRequest Request;
			Request.Source = GetOwner();
			Request.Target = Victim;
			Request.SkillId = TEXT("QRadiation");
			// Contact already locked execution. Healing never invalidates the captured target.
			Request.bExecute = bAttached;
			Request.HitLocation = GetGripLocation();
			GetWorld()->GetSubsystem<UCombatDamageSubsystem>()->ApplyDamage(Request);
		}
		Target.Reset();
		bAttached = false;
	}
	else if (Event == TEXT("SkillEnd"))
	{
		Finish();
	}
}

void UGurenQSkillComponent::FinishDash()
{
	if (Stage != EGurenQStage::Dash)
	{
		return;
	}
	const float Error = FVector::Dist2D(GetOwner()->GetActorLocation(), StandTransform.GetLocation());
	if (!Target.IsValid() || Error > ArrivalTolerance)
	{
		UE_LOG(LogGurenQ, Verbose, TEXT("Q blocked at %.1f cm from arrival"), Error);
		Cancel();
		return;
	}
	CastChecked<ACharacter>(GetOwner())->GetCharacterMovement()->StopMovementImmediately();
	SetStage(EGurenQStage::Grab);
}

void UGurenQSkillComponent::Finish()
{
	if (!IsQActive() || bFinishing)
	{
		return;
	}
	TGuardValue<bool> FinishingGuard(bFinishing, true);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Watchdog);
		World->GetTimerManager().ClearTimer(DashArrivalTimer);
	}
	SetComponentTickEnabled(false);
	// Restore presentation while the target still exists.
	SetStage(EGurenQStage::Idle);
	if (Target.IsValid())
	{
		Target->GetOwner()->OnDestroyed.RemoveDynamic(this, &UGurenQSkillComponent::TargetDestroyed);
		Target->Release(false);
	}
	Target.Reset();
	bAttached = false;
	ExecutionScale = 1.f;
	if (LockedController.IsValid())
	{
		LockedController->SetIgnoreMoveInput(false);
	}
	LockedController.Reset();
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetMesh()->SetRelativeTransform(OriginalMeshTransform);
		Character->GetCharacterMovement()->StopMovementImmediately();
		Character->GetCharacterMovement()->bOrientRotationToMovement = bOriginalOrientToMovement;
		if (UMotionWarpingComponent* Warp = Character->FindComponentByClass<UMotionWarpingComponent>())
		{
			Warp->RemoveWarpTarget(TEXT("Q_DashTarget"));
		}
	}
	RefreshTargets();
}

void UGurenQSkillComponent::Cancel()
{
	Finish();
}

void UGurenQSkillComponent::TargetDestroyed(AActor* DestroyedActor)
{
	Cancel();
}

void UGurenQSkillComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Cancel();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SelectionTimer);
	}
	Candidates.Reset();
	SelectedTarget.Reset();
	OnTargetsChanged.Broadcast();
	Super::EndPlay(Reason);
}

float UGurenQSkillComponent::GetExecuteThreshold(float MaxHealth) const
{
	return FMath::Max(FMath::Max(0.f, ExecuteHealthFlat), FMath::Max(0.f, MaxHealth) * FMath::Clamp(ExecuteHealthPercent, 0.f, 1.f));
}

bool UGurenQSkillComponent::CanExecuteTarget(AActor* Actor) const
{
	if (!UCombatDamageSubsystem::CanDamageTarget(GetOwner(), Actor))
	{
		return false;
	}
	const UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>();
	return Health && Health->GetHealth() <= GetExecuteThreshold(Health->GetMaxHealth());
}
