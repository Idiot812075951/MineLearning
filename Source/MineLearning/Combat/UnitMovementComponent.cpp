#include "UnitMovementComponent.h"
#include "CombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UUnitMovementComponent::UUnitMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUnitMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	Movement = Character ? Character->GetCharacterMovement() : nullptr;
	Combat = GetOwner()->FindComponentByClass<UCombatComponent>();
	if (Movement)
	{
		BaseWalkSpeed = Movement->MaxWalkSpeed;
	}
	if (Combat)
	{
		Combat->OnAttributesChanged.AddUniqueDynamic(this, &UUnitMovementComponent::UpdateSpeed);
	}
	UpdateSpeed();
}

void UUnitMovementComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Combat)
	{
		Combat->OnAttributesChanged.RemoveDynamic(this, &UUnitMovementComponent::UpdateSpeed);
	}
	Super::EndPlay(Reason);
}

void UUnitMovementComponent::SetLocomotionScale(float Scale)
{
	if (FMath::IsFinite(Scale) && Scale > 0.f && Scale != LocomotionScale)
	{
		LocomotionScale = Scale;
		UpdateSpeed();
	}
}

void UUnitMovementComponent::UpdateSpeed()
{
	if (Movement)
	{
		Movement->MaxWalkSpeed = FMath::Min(BaseWalkSpeed * LocomotionScale * (Combat ? Combat->GetMoveSpeedScale() : 1.f),
			Combat ? Combat->GetMaxMoveSpeed() : 1000.f);
	}
}
