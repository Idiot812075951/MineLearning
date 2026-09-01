#include "WarehouseScreenWidgetBase.h"

#include "MineLearning/MineLearningPlayerController.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/UI/WarehouseItemListItem.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "UObject/UnrealType.h"

int32 UWarehouseScreenWidgetBase::UpdateSelectionState(
	EItemType ItemType,
	int32 AvailableAmount,
	int32 FrozenAmount)
{
	const bool bSelectionChanged = !bHasSelectedItem
		|| SelectedItemType != ItemType;
	SelectedItemType = ItemType;
	SelectedAvailableAmount = AvailableAmount;
	SelectedFrozenAmount = FrozenAmount;
	bHasSelectedItem = true;
	NativeSelectedAmount = bSelectionChanged
		? 1
		: FMath::Clamp(NativeSelectedAmount, 1, GetSelectionMaximum());
	return NativeSelectedAmount;
}

int32 UWarehouseScreenWidgetBase::StepSelectionAmount(int32 Delta)
{
	NativeSelectedAmount = FMath::Clamp(
		NativeSelectedAmount + Delta,
		1,
		GetSelectionMaximum());
	return NativeSelectedAmount;
}

int32 UWarehouseScreenWidgetBase::MaximizeSelectionAmount()
{
	NativeSelectedAmount = GetSelectionMaximum();
	return NativeSelectedAmount;
}

int32 UWarehouseScreenWidgetBase::GetSelectionMaximum() const
{
	return bHasSelectedItem
		? FMath::Max(
			1,
			FMath::Max(SelectedAvailableAmount, SelectedFrozenAmount))
		: 1;
}

void UWarehouseScreenWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// These three controls share one authoritative native value. Removing only
	// this widget's legacy BP button bindings prevents display/state divergence
	// while the Widget Blueprint continues to own all presentation.
	if (MinusButton)
	{
		MinusButton->OnClicked.RemoveAll(this);
		MinusButton->OnClicked.AddUniqueDynamic(
			this, &UWarehouseScreenWidgetBase::HandleDecreaseClicked);
	}
	if (PlusButton)
	{
		PlusButton->OnClicked.RemoveAll(this);
		PlusButton->OnClicked.AddUniqueDynamic(
			this, &UWarehouseScreenWidgetBase::HandleIncreaseClicked);
	}
	if (MaxButton)
	{
		MaxButton->OnClicked.RemoveAll(this);
		MaxButton->OnClicked.AddUniqueDynamic(
			this, &UWarehouseScreenWidgetBase::HandleMaximumClicked);
	}
	if (ItemList)
	{
		ItemList->OnItemSelectionChanged().AddUObject(
			this, &UWarehouseScreenWidgetBase::HandleItemSelectionChanged);
	}

	if (AMineLearningPlayerController* Controller =
		Cast<AMineLearningPlayerController>(GetOwningPlayer()))
	{
		Warehouse = Controller->GetActiveWarehouse();
	}

	if (Warehouse)
	{
		Warehouse->OnWarehouseChanged.AddUniqueDynamic(
			this, &UWarehouseScreenWidgetBase::HandleWarehouseChanged);
	}

	OnWarehouseDataChanged();
}

void UWarehouseScreenWidgetBase::NativeDestruct()
{
	if (MinusButton)
	{
		MinusButton->OnClicked.RemoveDynamic(
			this, &UWarehouseScreenWidgetBase::HandleDecreaseClicked);
	}
	if (PlusButton)
	{
		PlusButton->OnClicked.RemoveDynamic(
			this, &UWarehouseScreenWidgetBase::HandleIncreaseClicked);
	}
	if (MaxButton)
	{
		MaxButton->OnClicked.RemoveDynamic(
			this, &UWarehouseScreenWidgetBase::HandleMaximumClicked);
	}
	if (ItemList)
	{
		ItemList->OnItemSelectionChanged().RemoveAll(this);
	}

	if (Warehouse)
	{
		Warehouse->OnWarehouseChanged.RemoveDynamic(
			this, &UWarehouseScreenWidgetBase::HandleWarehouseChanged);
		Warehouse = nullptr;
	}
	bHasSelectedItem = false;
	NativeSelectedAmount = 1;

	Super::NativeDestruct();
}

void UWarehouseScreenWidgetBase::HandleWarehouseChanged()
{
	OnWarehouseDataChanged();
}

void UWarehouseScreenWidgetBase::HandleDecreaseClicked()
{
	StepSelectionAmount(-1);
	SynchronizeSelectionAmount();
}

void UWarehouseScreenWidgetBase::HandleIncreaseClicked()
{
	StepSelectionAmount(1);
	SynchronizeSelectionAmount();
}

void UWarehouseScreenWidgetBase::HandleMaximumClicked()
{
	MaximizeSelectionAmount();
	SynchronizeSelectionAmount();
}

void UWarehouseScreenWidgetBase::HandleItemSelectionChanged(UObject* SelectedListItem)
{
	const UWarehouseItemListItem* WarehouseItem =
		Cast<UWarehouseItemListItem>(SelectedListItem);
	if (!WarehouseItem)
	{
		return;
	}

	const FWarehouseItemViewData& Data = WarehouseItem->GetData();
	UpdateSelectionState(Data.ItemType, Data.AvailableAmount, Data.FrozenAmount);
	SynchronizeSelectionAmount();
}

void UWarehouseScreenWidgetBase::SynchronizeSelectionAmount()
{
	if (SelectedAmountText)
	{
		SelectedAmountText->SetText(FText::AsNumber(NativeSelectedAmount));
	}

	// The existing WBP action handlers still read this authored variable. Keep
	// it synchronized until those handlers are migrated to the native getter.
	if (FIntProperty* SelectedAmountProperty = FindFProperty<FIntProperty>(
		GetClass(), TEXT("SelectedAmount")))
	{
		SelectedAmountProperty->SetPropertyValue_InContainer(
			this, NativeSelectedAmount);
	}
}
