#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ItemTypes.h"
#include "ItemReceiver.generated.h"

UINTERFACE(BlueprintType)
class MINELEARNING_API UItemReceiver : public UInterface
{
	GENERATED_BODY()
};

class MINELEARNING_API IItemReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item|Logistics")
	EItemReceiverType GetItemReceiverType() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item|Logistics")
	bool CanAcceptItem(const FItemStack& Item) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item|Logistics")
	bool AcceptItem(const FItemStack& Item);
};
