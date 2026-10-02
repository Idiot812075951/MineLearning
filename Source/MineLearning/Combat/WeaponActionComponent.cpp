#include "WeaponActionComponent.h"

UWeaponActionComponent::UWeaponActionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponActionComponent::Initialize(int32 Capacity)
{
	State.Capacity = FMath::Max(1, Capacity);
	State.Ammo = State.Capacity;
}

FWeaponShotContext UWeaponActionComponent::PrepareShot()
{
	FWeaponShotContext Context;
	OnPrepareShot.Broadcast(Context);
	return Context;
}

bool UWeaponActionComponent::ConsumeRound()
{
	if (!GetOwner()->HasAuthority() || State.Ammo <= 0)
	{
		return false;
	}
	--State.Ammo;
	return true;
}

void UWeaponActionComponent::CommitShot()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	OnShotCommitted.Broadcast();
	OnWeaponStateChanged.Broadcast();
}

void UWeaponActionComponent::CommitReload(int32 BaseCapacity)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	FReloadResultContext Context;
	Context.BaseCapacity = FMath::Max(1, BaseCapacity);
	Context.Capacity = Context.BaseCapacity;
	OnPrepareReload.Broadcast(Context);
	State.Capacity = FMath::Clamp(Context.Capacity, 1, 10000);
	State.Ammo = State.Capacity;
	++State.MagazineId;
	OnWeaponStateChanged.Broadcast();
}

void UWeaponActionComponent::Restore(const FWeaponRuntimeState& Saved)
{
	State = Saved;
	State.Capacity = FMath::Clamp(State.Capacity, 1, 10000);
	State.Ammo = FMath::Clamp(State.Ammo, 0, State.Capacity);
	OnWeaponStateChanged.Broadcast();
}

void UWeaponActionComponent::SetPattern(FName Source, int32 Rounds)
{
	Patterns.Add(Source, FMath::Clamp(Rounds, 1, 3));
	OnWeaponStateChanged.Broadcast();
}

void UWeaponActionComponent::RemovePattern(FName Source)
{
	Patterns.Remove(Source);
	OnWeaponStateChanged.Broadcast();
}

int32 UWeaponActionComponent::GetRoundsPerAttack() const
{
	if (!State.bBurstEnabled) { return 1; }
	int32 Result = 1;
	for (const TPair<FName, int32>& Pattern : Patterns)
	{
		Result = FMath::Max(Result, Pattern.Value);
	}
	return Result;
}

bool UWeaponActionComponent::HasBurstPattern() const
{
	for (const TPair<FName, int32>& Pattern : Patterns)
	{
		if (Pattern.Value > 1) { return true; }
	}
	return false;
}

bool UWeaponActionComponent::ToggleBurstMode()
{
	if (!GetOwner()->HasAuthority() || !HasBurstPattern()) { return false; }
	State.bBurstEnabled = !State.bBurstEnabled;
	OnWeaponStateChanged.Broadcast();
	return true;
}

int32 UWeaponActionComponent::GetCounter(FName Source) const
{
	const int32* Value = State.EffectCounters.Find(Source);
	return Value ? *Value : 0;
}

void UWeaponActionComponent::SetCounter(FName Source, int32 Value)
{
	State.EffectCounters.Add(Source, Value);
}

int32 UWeaponActionComponent::GetRemainingUntilSpecial() const
{
	int32 Result = 0;
	for (const TPair<FName, int32>& Preview : SpecialPreviews)
	{
		Result = Result == 0 ? Preview.Value : FMath::Min(Result, Preview.Value);
	}
	return Result;
}

void UWeaponActionComponent::SetSpecialPreview(FName Source, int32 Remaining)
{
	if (Remaining > 0)
	{
		SpecialPreviews.Add(Source, Remaining);
	}
	else
	{
		SpecialPreviews.Remove(Source);
	}
	OnWeaponStateChanged.Broadcast();
}
