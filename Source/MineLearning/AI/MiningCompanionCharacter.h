#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MiningCompanionCharacter.generated.h"

class UMiningToolComponent;
class UResourceCarryComponent;

UCLASS(BlueprintType)
class MINELEARNING_API AMiningCompanionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMiningCompanionCharacter();

	UFUNCTION(BlueprintPure, Category="Mining")
	UMiningToolComponent* GetMiningToolComponent() const { return MiningToolComponent; }

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	UResourceCarryComponent* GetResourceCarryComponent() const { return ResourceCarryComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	UMiningToolComponent* MiningToolComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Carry")
	UResourceCarryComponent* ResourceCarryComponent;
};
