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

void UResourceStorageComponent::BroadcastStorageChanged()
{
	OnStorageChanged.Broadcast(StoredOreCount);
}
