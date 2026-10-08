#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AutonomousUnit.generated.h"

enum class EAutonomousOutput : uint8 { None, PrimaryDamage, CarryCapacity };

UINTERFACE(MinimalAPI, BlueprintType)
class UAutonomousUnit : public UInterface
{
	GENERATED_BODY()
};

/** Opt-in capability. Having a mesh or a transformation form does not imply AI support. */
class MINELEARNING_API IAutonomousUnit
{
	GENERATED_BODY()
public:
	virtual bool SupportsAutonomousControl() const = 0;
	virtual EAutonomousOutput GetAutonomousOutput() const { return EAutonomousOutput::None; }
};
