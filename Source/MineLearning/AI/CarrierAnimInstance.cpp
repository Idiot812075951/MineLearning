#include "CarrierAnimInstance.h"

#include "HaulerCharacter.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"

void UCarrierAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	CachedHauler = Cast<AHaulerCharacter>(TryGetPawnOwner());
	bHasOwnerLocationSample = false;
}

void UCarrierAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!CachedHauler)
	{
		CachedHauler = Cast<AHaulerCharacter>(TryGetPawnOwner());
	}

	if (!CachedHauler)
	{
		GroundSpeed = 0.0f;
		bIsMoving = false;
		bHasCargo = false;
		bHasOwnerLocationSample = false;
		return;
	}

	// Normal MoveTo updates CharacterMovement velocity, while the narrow-aisle
	// recovery path deliberately moves the actor directly. Measure displacement
	// as well so the animation never falls back to Idle while the pawn is visibly
	// translating through that recovery path.
	const FVector CurrentOwnerLocation = CachedHauler->GetActorLocation();
	float ObservedSpeed = 0.0f;
	if (bHasOwnerLocationSample && DeltaSeconds > UE_SMALL_NUMBER)
	{
		ObservedSpeed = FVector::Dist2D(CurrentOwnerLocation, PreviousOwnerLocation) / DeltaSeconds;
	}
	PreviousOwnerLocation = CurrentOwnerLocation;
	bHasOwnerLocationSample = true;

	GroundSpeed = FMath::Max(CachedHauler->GetVelocity().Size2D(), ObservedSpeed);
	bIsMoving = GroundSpeed > 3.0f;
	const UResourceCarryComponent* Carry = CachedHauler->GetResourceCarryComponent();
	bHasCargo = Carry && !Carry->IsEmpty();
}

void UCarrierAnimInstance::AnimNotify_Pickup()
{
	if (CachedHauler)
	{
		CachedHauler->HandlePickupNotify();
	}
}

void UCarrierAnimInstance::AnimNotify_DropOff()
{
	if (CachedHauler)
	{
		CachedHauler->HandleDropOffNotify();
	}
}
