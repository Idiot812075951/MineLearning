#include "GrabbableComponent.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Pawn.h"

UGrabbableComponent::UGrabbableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

bool UGrabbableComponent::CanGrab(AActor* Requester) const
{
	const AActor* Target = GetOwner();
	return bGrabbable && IsRegistered() && IsValid(Requester) && IsValid(Target)
		&& Requester != Target && Target->HasAuthority() && !Target->IsActorBeingDestroyed()
		&& !Target->IsHidden() && !Target->GetAttachParentActor() && !Grabber.IsValid()
		&& !bReleasing && ComponentStates.IsEmpty()
		&& Target->GetRootComponent() && Target->GetRootComponent()->Mobility == EComponentMobility::Movable;
}

FBox UGrabbableComponent::GetMeshBounds() const
{
	FBox MeshBounds(ForceInit);
	TInlineComponentArray<UMeshComponent*> Meshes(GetOwner());
	for (const UMeshComponent* Mesh : Meshes)
	{
		if (Mesh->IsVisible() && !Mesh->bHiddenInGame && !Mesh->IsEditorOnly() && Mesh->Bounds.SphereRadius > UE_KINDA_SMALL_NUMBER)
		{
			MeshBounds += Mesh->Bounds.GetBox();
		}
	}
	return MeshBounds;
}

float UGrabbableComponent::GetGripDiameter() const
{
	if (GripDiameter > 0.f)
	{
		return GripDiameter * GetComponentScale().GetAbsMax();
	}
	const FBox MeshBounds = GetMeshBounds();
	return MeshBounds.IsValid ? FMath::Max(1.f, FMath::Min(MeshBounds.GetSize().X, MeshBounds.GetSize().Y)) : 1.f;
}

FVector UGrabbableComponent::GetIndicatorLocation() const
{
	const FBox MeshBounds = GetMeshBounds();
	const FVector Center = MeshBounds.IsValid ? MeshBounds.GetCenter() : GetComponentLocation();
	return FVector(Center.X, Center.Y, MeshBounds.IsValid ? MeshBounds.Max.Z : Center.Z) + FVector(0.f, 0.f, IndicatorHeight);
}

bool UGrabbableComponent::Reserve(AActor* Requester)
{
	if (!CanGrab(Requester))
	{
		return false;
	}
	AActor* Target = GetOwner();
	Grabber = Requester;
	OriginalTransform = Target->GetActorTransform();
	bActorTickEnabled = Target->IsActorTickEnabled();
	bCollisionEnabled = Target->GetActorEnableCollision();
	USceneComponent* Root = Target->GetRootComponent();
	bAbsoluteLocation = Root->IsUsingAbsoluteLocation();
	bAbsoluteRotation = Root->IsUsingAbsoluteRotation();
	bAbsoluteScale = Root->IsUsingAbsoluteScale();
	if (const APawn* Pawn = Cast<APawn>(Target))
	{
		if (AAIController* AI = Cast<AAIController>(Pawn->GetController()))
		{
			UBrainComponent* Brain = AI->GetBrainComponent();
			if (Brain && Brain->IsRunning() && !Brain->IsPaused())
			{
				Brain->PauseLogic(TEXT("Grabbed"));
				PausedBrain = Brain;
			}
		}
	}
	TInlineComponentArray<UActorComponent*> Components(Target);
	for (UActorComponent* Component : Components)
	{
		FComponentState& State = ComponentStates.AddDefaulted_GetRef();
		State.Component = Component;
		State.bTickEnabled = Component->IsComponentTickEnabled();
		Component->SetComponentTickEnabled(false);
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
		{
			State.bSimulating = Primitive->IsSimulatingPhysics();
			if (State.bSimulating)
			{
				State.LinearVelocity = Primitive->GetPhysicsLinearVelocity();
				State.AngularVelocity = Primitive->GetPhysicsAngularVelocityInRadians();
				Primitive->SetSimulatePhysics(false);
			}
		}
	}
	Target->SetActorTickEnabled(false);
	OnGrabStateChanged.Broadcast(Requester, true);
	return IsValid(Target) && !Target->IsActorBeingDestroyed() && Grabber.IsValid();
}

bool UGrabbableComponent::AttachToGrip(USceneComponent* Hand, FName Socket)
{
	AActor* Target = GetOwner();
	if (!Grabber.IsValid() || !IsValid(Hand) || Hand->GetOwner() != Grabber.Get() || !Hand->DoesSocketExist(Socket))
	{
		return false;
	}
	// Preserve the object's orientation and scale; offset the actor so this grip point, not its pivot, touches the hand.
	WorldGripOffset = GetComponentLocation() - Target->GetActorLocation();
	Target->SetActorEnableCollision(false);
	Target->SetActorLocation(Hand->GetSocketLocation(Socket) - WorldGripOffset, false, nullptr, ETeleportType::TeleportPhysics);
	Target->GetRootComponent()->SetAbsolute(true, true, true);
	if (!Target->AttachToComponent(Hand, FAttachmentTransformRules::KeepWorldTransform, Socket))
	{
		return false;
	}
	GripParent = Hand;
	GripSocket = Socket;
	AddTickPrerequisiteComponent(Hand);
	SetComponentTickEnabled(true);
	return true;
}

void UGrabbableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GripParent.IsValid())
	{
		// Absolute orientation keeps objects upright. Recompute translation after the hand pose
		// so an off-center grip does not orbit away when the hand rotates during the lift.
		GetOwner()->SetActorLocation(GripParent->GetSocketLocation(GripSocket) - WorldGripOffset, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UGrabbableComponent::Release(bool bCompleted)
{
	if (bReleasing || (!Grabber.IsValid() && ComponentStates.IsEmpty()))
	{
		return;
	}
	TGuardValue<bool> ReleasingGuard(bReleasing, true);
	AActor* Requester = Grabber.Get();
	AActor* Target = GetOwner();
	if (GripParent.IsValid())
	{
		RemoveTickPrerequisiteComponent(GripParent.Get());
	}
	GripParent.Reset();
	Target->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Target->GetRootComponent()->SetAbsolute(bAbsoluteLocation, bAbsoluteRotation, bAbsoluteScale);
	Target->SetActorTransform(OriginalTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Target->SetActorEnableCollision(bCollisionEnabled);
	Target->SetActorTickEnabled(bActorTickEnabled);
	for (const FComponentState& State : ComponentStates)
	{
		if (UActorComponent* Component = State.Component.Get())
		{
			Component->SetComponentTickEnabled(State.bTickEnabled);
			if (State.bSimulating)
			{
				if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
				{
					Primitive->SetSimulatePhysics(true);
					Primitive->SetPhysicsLinearVelocity(State.LinearVelocity);
					Primitive->SetPhysicsAngularVelocityInRadians(State.AngularVelocity);
				}
			}
		}
	}
	ComponentStates.Reset();
	Grabber.Reset();
	if (PausedBrain.IsValid())
	{
		PausedBrain->ResumeLogic(TEXT("Grab released"));
	}
	PausedBrain.Reset();
	OnGrabStateChanged.Broadcast(Requester, false);
	if (bCompleted && !Target->IsActorBeingDestroyed())
	{
		OnGrabCompleted.Broadcast(Requester);
	}
}

void UGrabbableComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Release(false);
	Super::EndPlay(Reason);
}
