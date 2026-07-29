#include "ResourceStorageComponent.h"

UResourceStorageComponent::UResourceStorageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UResourceStorageComponent::GetAvailableOre() const
{
	return FMath::Max(StoredOreCount - ReservedOreCount, 0);
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

bool UResourceStorageComponent::TryReserveOre(int32 Amount)
{
	if (Amount <= 0 || GetAvailableOre() < Amount)
	{
		return false;
	}

	ReservedOreCount += Amount;
	return true;
}

bool UResourceStorageComponent::CommitReservedOre(int32 Amount)
{
	if (Amount <= 0 || ReservedOreCount < Amount || StoredOreCount < Amount)
	{
		return false;
	}

	ReservedOreCount -= Amount;
	StoredOreCount -= Amount;
	BroadcastStorageChanged();
	return true;
}

bool UResourceStorageComponent::ReleaseReservedOre(int32 Amount)
{
	if (Amount <= 0 || ReservedOreCount < Amount)
	{
		return false;
	}

	ReservedOreCount -= Amount;
	BroadcastStorageChanged();
	return true;
}

void UResourceStorageComponent::BroadcastStorageChanged()
{
	OnStorageChanged.Broadcast(StoredOreCount);
}
