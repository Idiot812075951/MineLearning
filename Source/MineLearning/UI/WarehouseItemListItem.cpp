#include "WarehouseItemListItem.h"

UWarehouseItemListItem* UWarehouseItemListItem::CreateWarehouseListItem(
	UObject* Outer,
	const FWarehouseItemViewData& InData)
{
	if (!Outer)
	{
		return nullptr;
	}

	UWarehouseItemListItem* Item = NewObject<UWarehouseItemListItem>(Outer);
	Item->Data = InData;
	return Item;
}
