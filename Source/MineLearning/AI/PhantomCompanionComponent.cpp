#include "PhantomCompanionComponent.h"
#include "AutonomousUnit.h"
#include "UnitRetirementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "TimerManager.h"

UPhantomCompanionComponent::UPhantomCompanionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPhantomCompanionComponent::SpawnFor(APawn* Template, float Lifetime)
{
	if (!GetOwner()->HasAuthority())
	{
		return false;
	}
	const IAutonomousUnit* Capability = Cast<IAutonomousUnit>(Template);
	if (!IsValid(Template) || !Capability || !Capability->SupportsAutonomousControl())
	{
		Clear();
		return false;
	}
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector Location = Template->GetActorLocation() + Template->GetActorRightVector() * 180.f;
	APawn* NewUnit = GetWorld()->SpawnActor<APawn>(Template->GetClass(), Location, Template->GetActorRotation(), Params);
	if (!NewUnit) { return false; }
	NewUnit->SpawnDefaultController();
	if (!NewUnit->GetController())
	{
		NewUnit->Destroy();
		return false;
	}
	Clear();
	Companion = NewUnit;
	Companion->Tags.AddUnique(TEXT("Phantom"));
	UUnitRetirementComponent* Retirement = NewObject<UUnitRetirementComponent>(Companion);
	Companion->AddInstanceComponent(Retirement);
	Retirement->RegisterComponent();
	Companion->OnDestroyed.AddUniqueDynamic(this, &UPhantomCompanionComponent::CompanionDestroyed);
	GetWorld()->GetTimerManager().SetTimer(LifetimeTimer, this, &UPhantomCompanionComponent::LifetimeExpired, FMath::Max(Lifetime, 0.01f), false);
	OnCompanionChanged.Broadcast();
	return true;
}

void UPhantomCompanionComponent::Clear()
{
	GetWorld()->GetTimerManager().ClearTimer(LifetimeTimer);
	APawn* Previous = Companion;
	Companion = nullptr;
	if (IsValid(Previous))
	{
		Previous->OnDestroyed.RemoveDynamic(this, &UPhantomCompanionComponent::CompanionDestroyed);
		if (UUnitRetirementComponent* Retirement = Previous->FindComponentByClass<UUnitRetirementComponent>())
		{
			Retirement->Retire();
		}
		else
		{
			Previous->Destroy();
		}
	}
	OnCompanionChanged.Broadcast();
}

void UPhantomCompanionComponent::LifetimeExpired() { Clear(); }

void UPhantomCompanionComponent::CompanionDestroyed(AActor* Unit)
{
	if (Unit == Companion)
	{
		GetWorld()->GetTimerManager().ClearTimer(LifetimeTimer);
		Companion = nullptr;
		OnCompanionChanged.Broadcast();
	}
}

void UPhantomCompanionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Clear();
	Super::EndPlay(Reason);
}
