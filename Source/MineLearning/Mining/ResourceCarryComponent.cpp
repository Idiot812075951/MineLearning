#include "ResourceCarryComponent.h"

UResourceCarryComponent::UResourceCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UResourceCarryComponent::IsFull() const
{
	return CurrentOreCount >= MaxOreCount;
}

bool UResourceCarryComponent::CanAddOre(int32 Amount) const
{
	return Amount > 0 && CurrentOreCount < MaxOreCount;
}

int32 UResourceCarryComponent::AddOre(int32 Amount)
{
	if (!CanAddOre(Amount))
	{
		return 0;
	}

	const int32 OldCount = CurrentOreCount;
	const int32 AddAmount = FMath::Min(Amount, MaxOreCount - CurrentOreCount);
	CurrentOreCount += AddAmount;

	if (CurrentOreCount != OldCount)
	{
		BroadcastCarryChanged();
	}

	return AddAmount;
}

int32 UResourceCarryComponent::TakeAllOre()
{
	const int32 TakenAmount = CurrentOreCount;
	if (TakenAmount <= 0)
	{
		return 0;
	}

	CurrentOreCount = 0;
	BroadcastCarryChanged();
	return TakenAmount;
}

void UResourceCarryComponent::ClearOre()
{
	if (CurrentOreCount <= 0)
	{
		return;
	}

	CurrentOreCount = 0;
	BroadcastCarryChanged();
}

void UResourceCarryComponent::BroadcastCarryChanged()
{
	OnCarryChanged.Broadcast(CurrentOreCount, MaxOreCount);
}
