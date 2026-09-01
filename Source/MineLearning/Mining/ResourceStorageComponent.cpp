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
	return GetAvailableItemAmount(EItemType::IronOre);
}

int32 UResourceStorageComponent::GetStoredItemAmount(EItemType ItemType) const
{
	const int32* StoredAmount = StoredItems.Find(ItemType);
	return StoredAmount ? *StoredAmount : 0;
}

int32 UResourceStorageComponent::GetReservedItemAmount(EItemType ItemType) const
{
	const int32* ReservedAmount = ReservedItems.Find(ItemType);
	return ReservedAmount ? FMath::Max(*ReservedAmount, 0) : 0;
}

int32 UResourceStorageComponent::GetAvailableItemAmount(EItemType ItemType) const
{
	return FMath::Max(
		GetStoredItemAmount(ItemType) - GetReservedItemAmount(ItemType),
		0);
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

bool UResourceStorageComponent::RemoveItem(const FItemStack& Item)
{
	const int32 StoredAmount = GetStoredItemAmount(Item.ItemType);
	if (!Item.IsValid() || StoredAmount < Item.Amount)
	{
		return false;
	}

	if (StoredAmount - Item.Amount < GetReservedItemAmount(Item.ItemType))
	{
		return false;
	}

	const int32 RemainingAmount = StoredAmount - Item.Amount;
	if (RemainingAmount > 0)
	{
		StoredItems.FindOrAdd(Item.ItemType) = RemainingAmount;
	}
	else
	{
		StoredItems.Remove(Item.ItemType);
	}

	BroadcastStorageChanged();
	return true;
}

bool UResourceStorageComponent::TryReserveItem(const FItemStack& Item)
{
	if (!Item.IsValid() || GetAvailableItemAmount(Item.ItemType) < Item.Amount)
	{
		return false;
	}

	ReservedItems.FindOrAdd(Item.ItemType) += Item.Amount;
	BroadcastStorageChanged();
	return true;
}

bool UResourceStorageComponent::CanCommitReservedItem(const FItemStack& Item) const
{
	return Item.IsValid()
		&& GetReservedItemAmount(Item.ItemType) >= Item.Amount
		&& GetStoredItemAmount(Item.ItemType) >= Item.Amount;
}

bool UResourceStorageComponent::CommitReservedItem(const FItemStack& Item)
{
	if (!CanCommitReservedItem(Item))
	{
		return false;
	}

	const int32 RemainingReserved = GetReservedItemAmount(Item.ItemType) - Item.Amount;
	if (RemainingReserved > 0)
	{
		ReservedItems.FindOrAdd(Item.ItemType) = RemainingReserved;
	}
	else
	{
		ReservedItems.Remove(Item.ItemType);
	}

	const int32 RemainingStored = GetStoredItemAmount(Item.ItemType) - Item.Amount;
	if (RemainingStored > 0)
	{
		StoredItems.FindOrAdd(Item.ItemType) = RemainingStored;
	}
	else
	{
		StoredItems.Remove(Item.ItemType);
	}
	BroadcastStorageChanged();
	return true;
}

bool UResourceStorageComponent::ReleaseReservedItem(const FItemStack& Item)
{
	if (!Item.IsValid() || GetReservedItemAmount(Item.ItemType) < Item.Amount)
	{
		return false;
	}

	const int32 RemainingReserved = GetReservedItemAmount(Item.ItemType) - Item.Amount;
	if (RemainingReserved > 0)
	{
		ReservedItems.FindOrAdd(Item.ItemType) = RemainingReserved;
	}
	else
	{
		ReservedItems.Remove(Item.ItemType);
	}
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
	return TryReserveItem({EItemType::IronOre, Amount});
}

bool UResourceStorageComponent::CommitReservedOre(int32 Amount)
{
	return CommitReservedItem({EItemType::IronOre, Amount});
}

bool UResourceStorageComponent::ReleaseReservedOre(int32 Amount)
{
	return ReleaseReservedItem({EItemType::IronOre, Amount});
}

void UResourceStorageComponent::BroadcastStorageChanged()
{
	OnStorageChanged.Broadcast(GetStoredOreCount());
	OnInventoryChanged.Broadcast();
}
