#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MiningCompanionCharacter.generated.h"

class UMiningToolComponent;
class UNiagaraSystem;
class UResourceCarryComponent;
class USoundBase;

UCLASS(BlueprintType)
class MINELEARNING_API AMiningCompanionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMiningCompanionCharacter();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category="Mining")
	UMiningToolComponent* GetMiningToolComponent() const { return MiningToolComponent; }

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	UResourceCarryComponent* GetResourceCarryComponent() const { return ResourceCarryComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	UMiningToolComponent* MiningToolComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Carry")
	UResourceCarryComponent* ResourceCarryComponent;

	/** OreBuddy-only confirmed-hit sparks. Gameplay confirmation remains in MiningToolComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Feedback")
	TObjectPtr<UNiagaraSystem> MiningImpactSystem;

	/** Optional impact sound hook. Empty until a real project sound asset is supplied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Feedback")
	TObjectPtr<USoundBase> MiningImpactSound;

private:
	UFUNCTION()
	void HandleMiningHitConfirmed(FVector HitLocation, FVector HitNormal);
};
