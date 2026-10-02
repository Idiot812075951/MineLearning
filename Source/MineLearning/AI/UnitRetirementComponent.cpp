#include "UnitRetirementComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "TimerManager.h"

UUnitRetirementComponent::UUnitRetirementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUnitRetirementComponent::Retire()
{
	AActor* Unit = GetOwner();
	if (!Unit || !Unit->HasAuthority() || bRetiring || Unit->IsActorBeingDestroyed())
	{
		return;
	}
	bRetiring = true;
	Unit->SetActorEnableCollision(false);
	Unit->SetCanBeDamaged(false);
	Unit->SetActorTickEnabled(false);
	// Retirement stops gameplay immediately; the remaining lifespan is presentation only.
	GetWorld()->GetTimerManager().ClearAllTimersForObject(Unit);
	TArray<UActorComponent*> Components;
	Unit->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (Component != this)
		{
			GetWorld()->GetTimerManager().ClearAllTimersForObject(Component);
			Component->SetComponentTickEnabled(false);
		}
	}
	if (APawn* Pawn = Cast<APawn>(Unit))
	{
		if (UPawnMovementComponent* Movement = Pawn->GetMovementComponent())
		{
			Movement->StopMovementImmediately();
		}
		if (AAIController* AI = Cast<AAIController>(Pawn->GetController()))
		{
			AI->StopMovement();
			AI->UnPossess();
			AI->Destroy();
		}
	}
	OnRetiring.Broadcast();
	if (RetirementDuration > 0.f)
	{
		Unit->SetLifeSpan(RetirementDuration);
	}
	else
	{
		Unit->Destroy();
	}
}
