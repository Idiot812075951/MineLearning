#pragma once

#include "CoreMinimal.h"
#include "MineLearning/Mining/ItemTypes.h"
#include "Blueprint/UserWidget.h"
#include "WarehouseScreenWidgetBase.generated.h"

class AWarehouseDepot;
class UButton;
class UListView;
class UTextBlock;

/**
 * Native lifecycle bridge for the warehouse screen. It subscribes to gameplay
 * state and forwards change notifications; the derived Widget Blueprint owns
 * every visual and input update.
 */
UCLASS(Abstract, Blueprintable)
class MINELEARNING_API UWarehouseScreenWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Warehouse|UI")
	AWarehouseDepot* GetWarehouse() const { return Warehouse; }

	/**
	 * Refreshes the selected item's data while preserving the player's chosen
	 * amount when an inventory notification rebuilds the ListView.
	 */
	UFUNCTION(BlueprintCallable, Category="Warehouse|UI")
	int32 UpdateSelectionState(
		EItemType ItemType,
		int32 AvailableAmount,
		int32 FrozenAmount);

	UFUNCTION(BlueprintCallable, Category="Warehouse|UI")
	int32 StepSelectionAmount(int32 Delta);

	UFUNCTION(BlueprintCallable, Category="Warehouse|UI")
	int32 MaximizeSelectionAmount();

	UFUNCTION(BlueprintPure, Category="Warehouse|UI")
	int32 GetSelectionAmount() const { return NativeSelectedAmount; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category="Warehouse|UI")
	void OnWarehouseDataChanged();

private:
	UFUNCTION()
	void HandleWarehouseChanged();

	UFUNCTION()
	void HandleDecreaseClicked();

	UFUNCTION()
	void HandleIncreaseClicked();

	UFUNCTION()
	void HandleMaximumClicked();

	void HandleItemSelectionChanged(UObject* SelectedListItem);
	void SynchronizeSelectionAmount();

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, AllowPrivateAccess="true"))
	TObjectPtr<UButton> MinusButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, AllowPrivateAccess="true"))
	TObjectPtr<UButton> PlusButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, AllowPrivateAccess="true"))
	TObjectPtr<UButton> MaxButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, AllowPrivateAccess="true"))
	TObjectPtr<UListView> ItemList;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, AllowPrivateAccess="true"))
	TObjectPtr<UTextBlock> SelectedAmountText;

	UPROPERTY(Transient)
	TObjectPtr<AWarehouseDepot> Warehouse;

	EItemType SelectedItemType = EItemType::IronOre;
	int32 SelectedAvailableAmount = 0;
	int32 SelectedFrozenAmount = 0;
	int32 NativeSelectedAmount = 1;
	bool bHasSelectedItem = false;

	int32 GetSelectionMaximum() const;
};
