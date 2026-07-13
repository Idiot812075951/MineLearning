#include "ResourceStorageComponent.h"

UResourceStorageComponent::UResourceStorageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UResourceStorageComponent::AddOre(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	StoredOreCount += Amount;
	BroadcastStorageChanged();
	return Amount;
}

bool UResourceStorageComponent::CanConsumeOre(int32 Amount) const
{
	return Amount > 0 && StoredOreCount >= Amount;
}

bool UResourceStorageComponent::ConsumeOre(int32 Amount)
{
	if (!CanConsumeOre(Amount))
	{
		return false;
	}

	StoredOreCount -= Amount;
	BroadcastStorageChanged();
	return true;
}

void UResourceStorageComponent::BroadcastStorageChanged()
{
	OnStorageChanged.Broadcast(StoredOreCount);
}
