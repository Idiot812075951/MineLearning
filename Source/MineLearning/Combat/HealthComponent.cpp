#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHealthComponent::InitializeHealth(float Maximum)
{
	if (!GetOwner()->HasAuthority() || !FMath::IsFinite(Maximum))
	{
		return;
	}
	MaxHealth = FMath::Max(1.f, Maximum);
	Health = MaxHealth;
	OnHealthChanged.Broadcast();
}

void UHealthComponent::Heal(float Amount)
{
	if (!GetOwner()->HasAuthority() || IsDead() || !FMath::IsFinite(Amount) || Amount <= 0.f)
	{
		return;
	}
	const float Previous = Health;
	Health = FMath::Min(MaxHealth, Health + Amount);
	if (Health != Previous)
	{
		OnHealthChanged.Broadcast();
	}
}

FCombatDamageResult UHealthComponent::ApplyResolvedDamage(const FCombatDamageRequest& Request, float Damage)
{
	FCombatDamageResult Result;
	Result.bAccepted = true;
	Result.PreviousHealth = Health;
	Health = Request.bExecute ? 0.f : FMath::Max(Request.bNonLethal ? FMath::Min(1.f, Health) : 0.f, Health - Damage);
	Result.CurrentHealth = Health;
	Result.AppliedDamage = Result.PreviousHealth - Health;
	Result.bExecuted = Request.bExecute;
	OnHealthChanged.Broadcast();
	OnDamageResolved.Broadcast(Request, Result);
	if (IsDead() && bDestroyOnDeath && !GetOwner()->IsActorBeingDestroyed())
	{
		GetOwner()->Destroy();
	}
	return Result;
}

void UHealthComponent::OnRep_Health() { OnHealthChanged.Broadcast(); }

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHealthComponent, Health);
	DOREPLIFETIME(UHealthComponent, MaxHealth);
}
