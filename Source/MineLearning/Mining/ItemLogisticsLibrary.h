#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemRules.h"
#include "ItemLogisticsLibrary.generated.h"

class UDataTable;
class AActor;

UCLASS()
class MINELEARNING_API UItemLogisticsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Item|Logistics")
	static bool GetItemRule(const FItemStack& Item, FItemRuleRow& OutRule);

	UFUNCTION(BlueprintPure, Category="Item|Logistics")
	static EItemCategory GetItemCategory(EItemType ItemType);

	UFUNCTION(BlueprintPure, Category="Item|Economy")
	static int32 GetUnitSellPrice(EItemType ItemType);

	UFUNCTION(BlueprintCallable, Category="Item|Logistics", meta=(WorldContext="WorldContextObject"))
	static AActor* ResolveDestination(
		const UObject* WorldContextObject,
		const FItemStack& Item,
		const FVector& SearchOrigin);

	/**
	 * Routes receiver calls through a native interface address when one exists,
	 * while preserving Blueprint-only interface implementations as a fallback.
	 * This avoids UE 5.8 resolving native BlueprintNativeEvent interface calls to
	 * the interface default implementation.
	 */
	static bool TryGetReceiverType(AActor* Receiver, EItemReceiverType& OutReceiverType);

	static bool CanReceiverAcceptItem(AActor* Receiver, const FItemStack& Item);

	static bool DeliverItemToReceiver(AActor* Receiver, const FItemStack& Item);

private:
	static UDataTable* GetRulesTable();
	static FName GetRuleRowName(EItemType ItemType);
};
