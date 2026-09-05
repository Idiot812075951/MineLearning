#include "WarehouseScreenWidgetBase.h"

#include "MineLearning/MineLearningPlayerController.h"
#include "MineLearning/Mining/WarehouseDepot.h"

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
