#pragma once

#include "CoreMinimal.h"
#include "TransformationTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayerTransformationForm : uint8
{
	Human,
	OreBuddy,
	Gunner,
	Guren,
	Carrier
};
