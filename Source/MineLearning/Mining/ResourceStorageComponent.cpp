#include "ResourceStorageComponent.h"

UResourceStorageComponent::UResourceStorageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UResourceStorageComponent::GetStoredOreCount() const
{
	return GetStoredItemAmount(EItemType::IronOre);
}

int32 UResourceStorageComponent::GetAvailableOre() const
{
	return FMath::Max(GetStoredOreCount() - ReservedOreCount, 0);
}

int32 UResourceStorageComponent::GetStoredItemAmount(EItemType ItemType) const
{
	const int32* StoredAmount = StoredItems.Find(ItemType);
	return StoredAmount ? *StoredAmount : 0;
}

int32 UResourceStorageComponent::GetTotalStoredItemCount() const
{
	int32 Total = 0;
	for (const TPair<EItemType, int32>& Entry : StoredItems)
	{
		Total += FMath::Max(Entry.Value, 0);
	}
	return Total;
}

bool UResourceStorageComponent::CanAddItem(const FItemStack& Item) const
{
	return Item.IsValid()
		&& (MaxItemCapacity <= 0 || GetTotalStoredItemCount() + Item.Amount <= MaxItemCapacity);
}

bool UResourceStorageComponent::AddItem(const FItemStack& Item)
{
	if (!CanAddItem(Item))
	{
		return false;
	}

	StoredItems.FindOrAdd(Item.ItemType) += Item.Amount;
	BroadcastStorageChanged();
	return true;
}

int32 UResourceStorageComponent::AddOre(int32 Amount)
{
	FItemStack OreStack;
	OreStack.ItemType = EItemType::IronOre;
	OreStack.Amount = Amount;
	return AddItem(OreStack) ? Amount : 0;
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
	const int32 StoredOre = GetStoredOreCount();
	if (Amount <= 0 || ReservedOreCount < Amount || StoredOre < Amount)
	{
		return false;
	}

	ReservedOreCount -= Amount;
	const int32 RemainingOre = StoredOre - Amount;
	if (RemainingOre > 0)
	{
		StoredItems.FindOrAdd(EItemType::IronOre) = RemainingOre;
	}
	else
	{
		StoredItems.Remove(EItemType::IronOre);
	}
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
	OnStorageChanged.Broadcast(GetStoredOreCount());
}
