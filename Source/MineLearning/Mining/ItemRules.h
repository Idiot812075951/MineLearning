#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemTypes.h"
#include "ItemRules.generated.h"

USTRUCT(BlueprintType)
struct MINELEARNING_API FItemRuleRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Logistics")
	EItemCategory Category = EItemCategory::Misc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Logistics")
	TArray<EItemReceiverType> ReceiverPriority;
};
