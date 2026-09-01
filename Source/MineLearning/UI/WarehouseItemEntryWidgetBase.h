#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "WarehouseItemEntryWidgetBase.generated.h"

class UWarehouseItemListItem;

/**
 * Native ListView lifecycle bridge. Visual refresh remains implemented by the
 * derived Widget Blueprint through OnItemDataChanged.
 */
UCLASS(Abstract, Blueprintable)
class MINELEARNING_API UWarehouseItemEntryWidgetBase
	: public UUserWidget
	, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UFUNCTION(BlueprintImplementableEvent, Category="Warehouse|UI")
	void OnItemDataChanged(UWarehouseItemListItem* Item);
};
