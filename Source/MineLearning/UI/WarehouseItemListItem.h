#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "WarehouseItemListItem.generated.h"

UCLASS(BlueprintType)
class MINELEARNING_API UWarehouseItemListItem : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Warehouse|UI", meta=(DefaultToSelf="Outer"))
	static UWarehouseItemListItem* CreateWarehouseListItem(
		UObject* Outer,
		const FWarehouseItemViewData& InData);

	UFUNCTION(BlueprintPure, Category="Warehouse|UI")
	const FWarehouseItemViewData& GetData() const { return Data; }

private:
	UPROPERTY(BlueprintReadOnly, Category="Warehouse|UI", meta=(AllowPrivateAccess="true"))
	FWarehouseItemViewData Data;
};
