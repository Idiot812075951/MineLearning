#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GunnerCrosshairWidget.generated.h"

/**
 * Compatibility-only parent for existing assets created before UI logic moved
 * into Widget Blueprints. Keep this class presentation-free.
 */
UCLASS()
class MINELEARNING_API UGunnerCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()
};
