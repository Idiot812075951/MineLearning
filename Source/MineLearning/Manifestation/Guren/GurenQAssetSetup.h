#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GurenQAssetSetup.generated.h"

class USkeleton;

/** Narrow editor bridge for socket/slot APIs that are not exposed to editor Python. */
UCLASS()
class MINELEARNING_API UGurenQAssetSetup : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Q Editor")
	static void ConfigureSkeleton(USkeleton* Skeleton, FVector GripLocation);
};
