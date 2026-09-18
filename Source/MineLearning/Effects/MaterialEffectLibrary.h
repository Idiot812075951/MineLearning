#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MaterialEffectLibrary.generated.h"

class UMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class MINELEARNING_API UMaterialEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Creates a temporary instance of the current shader without modifying an existing gameplay MID. */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Material Effects")
	static UMaterialInstanceDynamic* CreateIsolatedMaterialInstance(UMeshComponent* Mesh, int32 ElementIndex);
};
