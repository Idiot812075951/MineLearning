#pragma once

#include "CoreMinimal.h"
#include "UnitEffectDefinition.h"
#include "ChargedMiningEffect.generated.h"

class UMiningToolComponent;
struct FMiningHitContext;

UCLASS(BlueprintType)
class MINELEARNING_API UChargedMiningEffectDefinition : public UUnitEffectDefinition
{
	GENERATED_BODY()
public:
	UChargedMiningEffectDefinition();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.01")) float ChargeSeconds = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float HitReduction = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float DamageMultiplier = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float ImpactScale = 1.5f;
	virtual bool ValidateDefinition(FString& Error) const override;
	virtual bool SupportsTarget(const AActor* Target) const override;
};

/** One charge powers an entire mining cycle; no visual or character dependencies. */
UCLASS()
class MINELEARNING_API UChargedMiningEffectInstance : public UUnitEffectInstance
{
	GENERATED_BODY()
public:
	virtual bool Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source) override;
	virtual void Revoke() override;
private:
	void StartCharge();
	void Charged();
	void CycleStarted();
	void CycleFinished();
	void PrepareHit(FMiningHitContext& Context);
	void HitCommitted();
	void Publish();
	UPROPERTY(Transient) TObjectPtr<UMiningToolComponent> Mining;
	FDelegateHandle StartedHandle;
	FDelegateHandle FinishedHandle;
	FDelegateHandle PrepareHandle;
	FDelegateHandle CommittedHandle;
	FTimerHandle ChargeTimer;
	bool bCharged = false;
	bool bCycleEligible = false;
	bool bCycleEmpowered = false;
};
