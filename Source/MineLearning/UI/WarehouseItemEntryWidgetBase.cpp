#include "WarehouseItemEntryWidgetBase.h"

#include "WarehouseItemListItem.h"

void UWarehouseItemEntryWidgetBase::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	OnItemDataChanged(Cast<UWarehouseItemListItem>(ListItemObject));
}
