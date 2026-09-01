#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemTypes.h"
#include "ItemRules.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct MINELEARNING_API FItemRuleRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Display")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Display")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Logistics")
	EItemCategory Category = EItemCategory::Misc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Logistics")
	TArray<EItemReceiverType> ReceiverPriority;

	/** Zero means this item cannot be sold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Economy", meta=(ClampMin="0"))
	int32 UnitSellPrice = 0;
};
