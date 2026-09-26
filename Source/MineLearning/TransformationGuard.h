#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TransformationGuard.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTransformationGuard : public UInterface
{
	GENERATED_BODY()
};

/** Forms derive this permission from their current actions; no separate lock state. */
class MINELEARNING_API ITransformationGuard
{
	GENERATED_BODY()
public:
	virtual bool CanTransform() const = 0;
};
