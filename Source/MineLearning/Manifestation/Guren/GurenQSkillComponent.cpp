#include "GurenQSkillComponent.h"
#include "QGrabTestDummy.h"
#include "Components/ArrowComponent.h"
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

AActor* UGurenQSkillComponent::GetTarget() const
{
	return Target.Get();
}

void UGurenQSkillComponent::SetStage(EGurenQStage NewStage)
{
	Stage = NewStage;
	UE_LOG(LogGurenQ, Display, TEXT("Q stage=%s target=%s"), *UEnum::GetValueAsString(Stage), *GetNameSafe(Target.Get()));
	OnStageChanged.Broadcast(Stage, Target.Get());
}

void UGurenQSkillComponent::TryCast()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (IsQActive() || !Character || !Character->GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}
	float Nearest = SelectionRange;
	for (TActorIterator<AQGrabTestDummy> It(GetWorld()); It; ++It)
	{
		const float Distance = FVector::Dist2D(Character->GetActorLocation(), It->GetActorLocation());
		if (Distance <= Nearest && !It->GetAttachParentActor())
		{
			Nearest = Distance;
			Target = *It;
		}
	}
	if (!Target.IsValid())
	{
		UE_LOG(LogGurenQ, Display, TEXT("Q rejected: no target within %.0f cm"), SelectionRange);
		return;
	}
	if (!Character->GetMesh()->DoesSocketExist(GripSocket))
	{
		UE_LOG(LogGurenQ, Error, TEXT("Q missing grip socket %s"), *GripSocket.ToString());
		Target.Reset();
		return;
	}
	OriginalTargetTransform = Target->GetActorTransform();
	bOriginalCollision = Target->GetActorEnableCollision();
	StandTransform = Target->GrabStandPoint->GetComponentTransform();
	FVector ApproachDirection = (Target->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
	if (ApproachDirection.IsNearlyZero())
	{
		ApproachDirection = Character->GetActorForwardVector().GetSafeNormal2D();
	}
	const FQuat ApproachRotation = ApproachDirection.Rotation().Quaternion();
	// Rotate the calibrated right-hand reach around the target, so approaches
	// from behind or either side do not inherit the target's fixed facing.
	const FQuat AlignmentRotation = ApproachRotation * StandTransform.GetRotation().Inverse();
	const FVector StandOffset = StandTransform.GetLocation() - Target->GetActorLocation();
	FVector StandLocation = Target->GetActorLocation() + AlignmentRotation.RotateVector(StandOffset);
	StandLocation.Z = Character->GetActorLocation().Z;
	StandTransform.SetRotation(ApproachRotation);
	StandTransform.SetLocation(StandLocation);
	Character->StopJumping();
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->ConsumeMovementInputVector();
	bOriginalOrientToMovement = Character->GetCharacterMovement()->bOrientRotationToMovement;
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->SetActorRotation(StandTransform.Rotator());
	LockedController = Character->GetController();
	if (LockedController.IsValid())
	{
		LockedController->SetIgnoreMoveInput(true);
	}
	Target->OnDestroyed.AddDynamic(this, &UGurenQSkillComponent::TargetDestroyed);
	if (UMotionWarpingComponent* Warp = Character->FindComponentByClass<UMotionWarpingComponent>())
	{
		Warp->AddOrUpdateWarpTargetFromTransform(TEXT("Q_DashTarget"), StandTransform);
	}
	SetComponentTickEnabled(true);
	GetWorld()->GetTimerManager().SetTimer(Watchdog, this, &UGurenQSkillComponent::Cancel, 8.f, false);
	SetStage(Nearest > DirectGrabRange ? EGurenQStage::Dash : EGurenQStage::Grab);
}

void UGurenQSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Target.IsValid() && Stage != EGurenQStage::Release)
	{
		Cancel();
		return;
	}
	// Only the short direct-grab reach is aligned here. The dash is entirely root motion.
	if (Stage == EGurenQStage::Grab && !bAttached)
	{
		GetOwner()->SetActorLocation(FMath::VInterpTo(GetOwner()->GetActorLocation(), StandTransform.GetLocation(), DeltaTime, 25.f), true);
	}
}

void UGurenQSkillComponent::HandleAnimationEvent(FName Event)
{
	if (!IsQActive())
	{
		return;
	}
	ACharacter* Character = CastChecked<ACharacter>(GetOwner());
	if (Event == TEXT("DashArrival") && Stage == EGurenQStage::Dash)
	{
		// Notifies run while extracting the pose, before CharacterMovement applies this
		// frame's root motion. Check the endpoint after movement, including at low FPS.
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGurenQSkillComponent::FinishDash);
	}
	else if (Event == TEXT("GrabContact") && Stage == EGurenQStage::Grab && !bAttached)
	{
		if (!Target.IsValid() || FVector::Dist(Character->GetMesh()->GetSocketLocation(GripSocket), Target->GetActorLocation()) > 100.f)
		{
			Cancel();
			return;
		}
		Target->SetActorEnableCollision(false);
		Target->GetRootComponent()->SetAbsolute(false, true, true);
		Target->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false), GripSocket);
		bAttached = true;
		OnGrabContact.Broadcast();
		UE_LOG(LogGurenQ, Display, TEXT("Q GrabContact attached to right claw"));
	}
	else if (Event == TEXT("StartDissolve") && Stage == EGurenQStage::Grab && bAttached)
	{
		SetStage(EGurenQStage::Radiation);
	}
	else if (Event == TEXT("DissolveFinish") && Stage == EGurenQStage::Radiation)
	{
		SetStage(EGurenQStage::Release);
		if (Target.IsValid())
		{
			Target->OnDestroyed.RemoveDynamic(this, &UGurenQSkillComponent::TargetDestroyed);
			Target->Destroy();
		}
		Target.Reset();
		bAttached = false;
	}
	else if (Event == TEXT("SkillEnd"))
	{
		Finish(Stage != EGurenQStage::Release);
	}
}

void UGurenQSkillComponent::FinishDash()
{
	if (Stage != EGurenQStage::Dash)
	{
		return;
	}
	ACharacter* Character = CastChecked<ACharacter>(GetOwner());
	const float Error = FVector::Dist2D(Character->GetActorLocation(), StandTransform.GetLocation());
	if (!Target.IsValid() || Error > 55.f)
	{
		UE_LOG(LogGurenQ, Warning, TEXT("Q dash obstructed or warp endpoint missed: %.1f cm"), Error);
		Cancel();
		return;
	}
	UE_LOG(LogGurenQ, Display, TEXT("Q warp endpoint error %.1f cm"), Error);
	Character->GetCharacterMovement()->StopMovementImmediately();
	SetStage(EGurenQStage::Grab);
}

void UGurenQSkillComponent::Finish(bool bCancelled)
{
	if (!IsQActive())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(Watchdog);
	SetComponentTickEnabled(false);
	// Subscribers reset their effects while the target still exists.
	SetStage(EGurenQStage::Idle);
	if (Target.IsValid())
	{
		Target->OnDestroyed.RemoveDynamic(this, &UGurenQSkillComponent::TargetDestroyed);
		if (bAttached)
		{
			Target->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			Target->GetRootComponent()->SetAbsolute(false, false, false);
		}
		Target->SetActorTransform(OriginalTargetTransform);
		Target->SetActorEnableCollision(bOriginalCollision);
	}
	Target.Reset();
	bAttached = false;
	if (LockedController.IsValid())
	{
		LockedController->SetIgnoreMoveInput(false);
	}
	LockedController.Reset();
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
		Character->GetCharacterMovement()->bOrientRotationToMovement = bOriginalOrientToMovement;
		if (UMotionWarpingComponent* Warp = Character->FindComponentByClass<UMotionWarpingComponent>())
		{
			Warp->RemoveWarpTarget(TEXT("Q_DashTarget"));
		}
	}
	UE_LOG(LogGurenQ, Display, TEXT("Q %s; movement restored"), bCancelled ? TEXT("cancelled") : TEXT("complete"));
}

void UGurenQSkillComponent::Cancel()
{
	Finish(true);
}

void UGurenQSkillComponent::TargetDestroyed(AActor* DestroyedActor)
{
	Cancel();
}

void UGurenQSkillComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Cancel();
	Super::EndPlay(Reason);
}
