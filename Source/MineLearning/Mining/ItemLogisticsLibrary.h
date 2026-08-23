#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemRules.h"
#include "ItemLogisticsLibrary.generated.h"

class UDataTable;

UCLASS()
class MINELEARNING_API UItemLogisticsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Item|Logistics")
	static bool GetItemRule(const FItemStack& Item, FItemRuleRow& OutRule);

	UFUNCTION(BlueprintPure, Category="Item|Logistics")
	static EItemCategory GetItemCategory(EItemType ItemType);

	UFUNCTION(BlueprintCallable, Category="Item|Logistics", meta=(WorldContext="WorldContextObject"))
	static AActor* ResolveDestination(
		const UObject* WorldContextObject,
		const FItemStack& Item,
		const FVector& SearchOrigin);

private:
	static UDataTable* GetRulesTable();
	static FName GetRuleRowName(EItemType ItemType);
};
