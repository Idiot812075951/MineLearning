#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MiningCompanionCharacter.generated.h"

class UMiningToolComponent;

UCLASS()
class MINELEARNING_API AMiningCompanionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMiningCompanionCharacter();

	UMiningToolComponent* GetMiningToolComponent() const { return MiningToolComponent; }
	
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	UMiningToolComponent* MiningToolComponent;
};