#include "GurenUltimateComponent.h"
#include "GurenQSkillComponent.h"
#include "MineLearning/Interaction/GrabbableComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogGurenUltimate, Log, All);

namespace
{
FBox VisibleGeometryBounds(AActor* Actor)
{
	FBox Bounds(ForceInit);
	TInlineComponentArray<UMeshComponent*> Meshes(Actor);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (!Mesh->IsEditorOnly() && Mesh->IsVisible() && !Mesh->bHiddenInGame
			&& (Mesh->IsA<USkeletalMeshComponent>() || Mesh->IsA<UStaticMeshComponent>()))
		{
			Bounds += Mesh->Bounds.GetBox();
		}
	}
	return Bounds;
}
}

UGurenUltimateComponent::UGurenUltimateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UGurenUltimateComponent::IsValidTarget(AActor* Candidate) const
{
	if (!IsValid(Candidate) || Candidate == GetOwner() || Candidate->IsActorBeingDestroyed()
		|| FVector::DistSquared2D(Candidate->GetActorLocation(), GetOwner()->GetActorLocation()) > FMath::Square(Range))
	{
		return false;
	}
	const UGrabbableComponent* Grab = Candidate->FindComponentByClass<UGrabbableComponent>();
	if (!Grab || !Grab->CanGrab(GetOwner()))
	{
		return false;
	}
	const FBox Bounds = VisibleGeometryBounds(Candidate);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ArrivalTarget), false, GetOwner());
	Params.AddIgnoredActor(Candidate);
	return !GetWorld()->LineTraceTestByChannel(GetOwner()->GetActorLocation(),
		Bounds.IsValid ? Bounds.GetCenter() : Candidate->GetActorLocation(), ECC_Visibility, Params);
}

void UGurenUltimateComponent::AddTarget(AActor* Actor)
{
	UGrabbableComponent* Grab = Actor->FindComponentByClass<UGrabbableComponent>();
	if (!Grab || !Grab->Reserve(GetOwner()))
	{
		return;
	}
	FArrivalTarget& Entry = Targets.AddDefaulted_GetRef();
	Entry.Actor = Actor;
	Entry.Reservation = Grab;
	Entry.bDamageEnabled = Actor->CanBeDamaged();
	Actor->SetCanBeDamaged(false);
	const FBox Bounds = VisibleGeometryBounds(Actor);
	Entry.Location = Bounds.IsValid ? Bounds.GetCenter() : Actor->GetActorLocation();
	Entry.Radius = Bounds.IsValid ? FMath::Clamp(static_cast<float>(Bounds.GetExtent().GetMax()), 35.f, 550.f) : 100.f;
	TInlineComponentArray<UMeshComponent*> Meshes(Actor);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (!Mesh->IsEditorOnly() && !Mesh->bHiddenInGame && Mesh->DoesSocketExist(HitSocket))
		{
			Entry.Location = Mesh->GetSocketLocation(HitSocket);
			break;
		}
	}
}

bool UGurenUltimateComponent::TryStart(AActor* RequestedTarget)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (IsUltimateActive() || bExiting || !Character || !Character->HasAuthority() || !Character->IsLocallyControlled())
	{
		return false;
	}
	const UGurenQSkillComponent* Q = Character->FindComponentByClass<UGurenQSkillComponent>();
	if (Q && Q->IsQActive())
	{
		return false;
	}
	AActor* Chosen = RequestedTarget;
	if (!Chosen && Q && Q->GetSelectedTarget())
	{
		Chosen = Q->GetSelectedTarget()->GetOwner();
	}
	if (!Chosen)
	{
		float Best = FMath::Square(Range);
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			const float Distance = FVector::DistSquared2D(It->GetActorLocation(), Character->GetActorLocation());
			if (Distance < Best && IsValidTarget(*It))
			{
				Chosen = *It;
				Best = Distance;
			}
		}
	}
	if (!IsValidTarget(Chosen))
	{
		return false;
	}
	Targets.Reset();
	AddTarget(Chosen);
	if (Targets.IsEmpty())
	{
		return false;
	}
	// Greedy nearest-neighbour order is stable for a cast; destroyed targets retain their cached waypoint.
	TSet<AActor*> Considered;
	Considered.Add(Chosen);
	while (Targets.Num() < FMath::Clamp(MaxTargets, 1, 12))
	{
		AActor* Next = nullptr;
		float Best = FMath::Square(ChainRange);
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (Considered.Contains(*It) || !IsValidTarget(*It))
			{
				continue;
			}
			const float Distance = FVector::DistSquared(It->GetActorLocation(), Targets.Last().Location);
			if (Distance < Best)
			{
				Next = *It;
				Best = Distance;
			}
		}
		if (!Next)
		{
			break;
		}
		Considered.Add(Next);
		AddTarget(Next);
	}
	OriginalTransform = Character->GetActorTransform();
	AttackDirection = (GetTargetBounds().GetCenter() - OriginalTransform.GetLocation()).GetSafeNormal2D();
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = Character->GetActorForwardVector();
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ArrivalPlacement), false, Character);
	for (const FArrivalTarget& Entry : Targets)
	{
		Params.AddIgnoredActor(Entry.Actor.Get());
	}
	const FVector Start = Character->GetActorLocation();
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FHitResult Ground;
	bHasGround = GetWorld()->LineTraceSingleByChannel(Ground, Start + FVector(0, 0, 100),
		Start - FVector(0, 0, 20000), ECC_Visibility, Params) && Ground.ImpactNormal.Z > 0.65f;
	LaunchLocation = FVector(Start.X, Start.Y, bHasGround ? Ground.ImpactPoint.Z + HalfHeight + LaunchHeight : Start.Z);
	ArrivalLocation = FVector(Start.X, Start.Y, bHasGround ? Ground.ImpactPoint.Z + HalfHeight + FinalHoverHeight : Start.Z);
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(Character->GetCapsuleComponent()->GetScaledCapsuleRadius(), HalfHeight);
	FHitResult Ceiling;
	if (GetWorld()->SweepSingleByChannel(Ceiling, Start, LaunchLocation, FQuat::Identity, ECC_Pawn, Capsule, Params))
	{
		LaunchLocation = FMath::Lerp(Start, LaunchLocation, FMath::Max(0.f, Ceiling.Time - 0.03f));
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	OriginalMovementMode = Move->MovementMode;
	OriginalCustomMode = Move->CustomMovementMode;
	OriginalVelocity = Move->Velocity;
	bOriginalOrient = Move->bOrientRotationToMovement;
	bOriginalDamage = Character->CanBeDamaged();
	bOriginalInputEnabled = Character->InputEnabled();
	Player = Cast<APlayerController>(Character->GetController());
	Character->StopJumping();
	Character->ConsumeMovementInputVector();
	Move->StopMovementImmediately();
	Move->DisableMovement();
	Move->bOrientRotationToMovement = false;
	Character->SetCanBeDamaged(false);
	if (Player.IsValid())
	{
		// Pawn input is assembled separately from the controller's additional input stack.
		Character->DisableInput(Player.Get());
		Player->SetIgnoreMoveInput(true);
		Player->SetIgnoreLookInput(true);
	}
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		if (*It == Character || It->IsPlayerControlled() || Considered.Contains(*It)
			|| FVector::DistSquared(It->GetActorLocation(), Start) > FMath::Square(Range * 1.5f))
		{
			continue;
		}
		FHold& Hold = Holds.AddDefaulted_GetRef();
		Hold.Actor = *It;
		Hold.bTick = It->IsActorTickEnabled();
		Hold.TimeDilation = It->CustomTimeDilation;
		It->SetActorTickEnabled(false);
		It->CustomTimeDilation = 0.f;
		if (AAIController* AI = Cast<AAIController>(It->GetController()))
		{
			UBrainComponent* Brain = AI->GetBrainComponent();
			if (Brain && Brain->IsRunning() && !Brain->IsPaused())
			{
				Brain->PauseLogic(TEXT("Arrival"));
				Hold.Brain = Brain;
			}
			FHold& ControllerHold = Holds.AddDefaulted_GetRef();
			ControllerHold.Actor = AI;
			ControllerHold.bTick = AI->IsActorTickEnabled();
			ControllerHold.TimeDilation = AI->CustomTimeDilation;
			AI->SetActorTickEnabled(false);
		}
	}
	bCommitted = false;
	SetComponentTickEnabled(true);
	SetStage(EGurenUltimateStage::Ready);
	return IsUltimateActive();
}

float UGurenUltimateComponent::GetStageDuration(EGurenUltimateStage InStage) const
{
	const int32 Index = static_cast<int32>(InStage) - 1;
	const float Duration = StageDurations.IsValidIndex(Index) ? FMath::Max(0.1f, StageDurations[Index]) : 0.5f;
	return InStage == EGurenUltimateStage::Launch || InStage == EGurenUltimateStage::Impact
		? Duration / FMath::Max(0.1f, FlightSpeedMultiplier) : Duration;
}

FVector UGurenUltimateComponent::EvaluateArc(const FVector& Start, const FVector& End, const FVector& Side, float Width, float Alpha)
{
	const float A = FMath::Clamp(Alpha, 0.f, 1.f);
	const FVector Delta = End - Start;
	const float Bend = FMath::Min(FMath::Max(0.f, Width), static_cast<float>(Delta.Length()) * 0.15f);
	// One shallow bow per transfer. Progress along the chord is monotonic, so it cannot orbit a target.
	const FVector Lateral = FVector::VectorPlaneProject(Side, Delta.GetSafeNormal()).GetSafeNormal();
	return FMath::Lerp(Start, End, A) + Lateral * (4.f * A * (1.f - A) * Bend);
}

FVector UGurenUltimateComponent::GetFlightLocation() const
{
	if (Targets.IsEmpty())
	{
		return FlightOrigin;
	}
	if (Stage == EGurenUltimateStage::Impact)
	{
		const ACharacter* Character = CastChecked<ACharacter>(GetOwner());
		const FVector End = Character->GetMesh()->GetSocketLocation(LaunchSocket);
		return EvaluateArc(Targets.Last().Location, End, -FVector::CrossProduct(FVector::UpVector, AttackDirection),
			ArcWidth, StageTime / GetStageDuration(Stage));
	}
	const float Position = FMath::Clamp(StageTime / GetStageDuration(EGurenUltimateStage::Launch), 0.f, 1.f) * Targets.Num();
	const int32 Index = FMath::Min(FMath::FloorToInt(Position), Targets.Num() - 1);
	const FVector Start = Index == 0 ? FlightOrigin : Targets[Index - 1].Location;
	const FVector End = Targets[Index].Location;
	FVector Side = FVector::CrossProduct(FVector::UpVector, (End - Start).GetSafeNormal()).GetSafeNormal();
	if (Side.IsNearlyZero())
	{
		Side = FVector::RightVector;
	}
	return EvaluateArc(Start, End, Side * (Index % 2 == 0 ? 1.f : -1.f), Index == 0 ? 0.f : ArcWidth, Position - Index);
}

FBox UGurenUltimateComponent::GetTargetBounds() const
{
	FBox Bounds(ForceInit);
	for (const FArrivalTarget& Entry : Targets)
	{
		Bounds += FBox(Entry.Location - FVector(Entry.Radius), Entry.Location + FVector(Entry.Radius));
	}
	return Bounds.IsValid ? Bounds : FBox(GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation());
}

void UGurenUltimateComponent::SetStage(EGurenUltimateStage NewStage)
{
	Stage = NewStage;
	StageTime = 0.f;
	if (Stage == EGurenUltimateStage::Launch)
	{
		FlightOrigin = CastChecked<ACharacter>(GetOwner())->GetMesh()->GetSocketLocation(LaunchSocket);
	}
	UE_LOG(LogGurenUltimate, Display, TEXT("Arrival %s targets=%d"), *UEnum::GetValueAsString(Stage), Targets.Num());
	OnStageChanged.Broadcast(Stage);
}

void UGurenUltimateComponent::HandleBeat(EGurenUltimateStage Beat)
{
	if (!IsUltimateActive() || bExiting)
	{
		return;
	}
	if (Beat == EGurenUltimateStage::Idle && Stage == EGurenUltimateStage::Recover)
	{
		Exit(false);
		return;
	}
	if (static_cast<uint8>(Beat) != static_cast<uint8>(Stage) + 1)
	{
		return;
	}
	StageTime = GetStageDuration(Stage);
	UpdatePhase();
	if (!IsUltimateActive() || bExiting)
	{
		return;
	}
	SetStage(Beat);
	if (Stage != Beat || bExiting)
	{
		return;
	}
	if (Beat == EGurenUltimateStage::Burst && !bCommitted)
	{
		bCommitted = true;
		for (FArrivalTarget& Entry : Targets)
		{
			if (Entry.Actor.IsValid())
			{
				Entry.Actor->SetCanBeDamaged(Entry.bDamageEnabled);
			}
			if (Entry.Reservation.IsValid())
			{
				Entry.Reservation->Release(Entry.bPierced);
			}
			Entry.Reservation.Reset();
		}
	}
}

void UGurenUltimateComponent::UpdatePhase()
{
	ACharacter* Character = CastChecked<ACharacter>(GetOwner());
	const float Alpha = FMath::Clamp(StageTime / GetStageDuration(Stage), 0.f, 1.f);
	if (Stage == EGurenUltimateStage::Ready)
	{
		Character->SetActorLocation(FMath::Lerp(OriginalTransform.GetLocation(), LaunchLocation, FMath::SmoothStep(0.f, 1.f, Alpha)), true);
		Character->SetActorRotation(AttackDirection.Rotation());
	}
	else if (Stage == EGurenUltimateStage::Launch)
	{
		for (int32 Index = 0; Index < Targets.Num(); ++Index)
		{
			if (!Targets[Index].bPierced && Alpha * Targets.Num() + UE_KINDA_SMALL_NUMBER >= Index + 1)
			{
				Targets[Index].bPierced = true;
				OnTargetPierced.Broadcast(Index);
				if (!IsUltimateActive())
				{
					return;
				}
			}
		}
	}
	else if (Stage == EGurenUltimateStage::Descent)
	{
		const float Blend = FMath::SmoothStep(0.f, 1.f, Alpha);
		Character->SetActorLocation(FMath::Lerp(LaunchLocation, ArrivalLocation, Blend), true);
		Character->SetActorRotation(FQuat::Slerp(AttackDirection.Rotation().Quaternion(), (-AttackDirection).Rotation().Quaternion(), Blend));
	}
}

void UGurenUltimateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	if (!CastChecked<ACharacter>(GetOwner())->IsLocallyControlled())
	{
		Abort();
		return;
	}
	StageTime += DeltaTime;
	while (IsUltimateActive() && StageTime >= GetStageDuration(Stage))
	{
		const float Remaining = StageTime - GetStageDuration(Stage);
		HandleBeat(Stage == EGurenUltimateStage::Recover ? EGurenUltimateStage::Idle
			: static_cast<EGurenUltimateStage>(static_cast<uint8>(Stage) + 1));
		if (IsUltimateActive())
		{
			StageTime = Remaining;
		}
	}
	if (IsUltimateActive())
	{
		UpdatePhase();
	}
}

void UGurenUltimateComponent::Exit(bool bCancelled)
{
	if (!IsUltimateActive() || bExiting)
	{
		return;
	}
	TGuardValue<bool> Guard(bExiting, true);
	SetStage(EGurenUltimateStage::Idle);
	for (FArrivalTarget& Entry : Targets)
	{
		if (Entry.Actor.IsValid())
		{
			Entry.Actor->SetCanBeDamaged(Entry.bDamageEnabled);
		}
		if (Entry.Reservation.IsValid())
		{
			Entry.Reservation->Release(false);
		}
		Entry.Reservation.Reset();
	}
	for (const FHold& Hold : Holds)
	{
		if (Hold.Actor.IsValid())
		{
			Hold.Actor->SetActorTickEnabled(Hold.bTick);
			Hold.Actor->CustomTimeDilation = Hold.TimeDilation;
		}
		if (Hold.Brain.IsValid())
		{
			Hold.Brain->ResumeLogic(TEXT("Arrival ended"));
		}
	}
	Holds.Reset();
	ACharacter* Character = CastChecked<ACharacter>(GetOwner());
	Character->SetCanBeDamaged(bOriginalDamage);
	if (bCancelled)
	{
		Character->SetActorTransform(OriginalTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	Move->SetMovementMode(bCancelled ? OriginalMovementMode : MOVE_Flying, OriginalCustomMode);
	Move->bOrientRotationToMovement = bOriginalOrient;
	Move->Velocity = bCancelled ? OriginalVelocity : FVector::ZeroVector;
	if (Player.IsValid())
	{
		if (bOriginalInputEnabled)
		{
			Character->EnableInput(Player.Get());
		}
		Player->SetIgnoreMoveInput(false);
		Player->SetIgnoreLookInput(false);
	}
	Player.Reset();
	SetComponentTickEnabled(false);
}

void UGurenUltimateComponent::Abort() { Exit(true); }
void UGurenUltimateComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Abort();
	Super::EndPlay(Reason);
}
