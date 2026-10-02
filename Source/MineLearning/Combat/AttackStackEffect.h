#pragma once

#include "CoreMinimal.h"
#include "UnitEffectDefinition.h"
#include "AttackStackEffect.generated.h"

/** One shared hit counter drives all per-stack and full-stack bonuses. */
UCLASS(BlueprintType)
class MINELEARNING_API UAttackStackEffectDefinition : public UUnitEffectDefinition
{
	GENERATED_BODY()
public:
	UAttackStackEffectDefinition();
	/** Zero means no stack limit. Attribute caps still apply at the action boundary. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacks", meta=(ClampMin="0")) int32 MaxStacks = 8;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacks", meta=(ClampMin="0.01")) float DecayInterval = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacks") bool bRequiresMovement = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacks") bool bClearOnMiss = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacks") FCombatModifiers FullStackModifiers;
	/** Present while equipped, including at zero stacks (e.g. AI miss chance). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacks") FCombatModifiers EquippedModifiers;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacks", meta=(ClampMin="1")) int32 StacksPerHit = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacks", meta=(ClampMin="1")) int32 StacksPerStrafeHit = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0")) float AuraPerStack = 0.f;
	virtual bool ValidateDefinition(FString& Error) const override;
	virtual FText GetRuleDescription() const override;
};

UCLASS()
class MINELEARNING_API UAttackStackEffectInstance : public UUnitEffectInstance
{
	GENERATED_BODY()
public:
	virtual bool Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source) override;
	virtual void Revoke() override;
	virtual void HandleEvent(FName EventName) override {}
	virtual bool HandleExpiry() override;
private:
	void AttackResolved(bool bHit, bool bMoving);
	void ApplyStacks(float Duration);
	FName GetEquippedSource() const;
	UPROPERTY(Transient) TObjectPtr<UCombatComponent> Combat;
	FDelegateHandle AttackHandle;
	int32 Stacks = 0;
};
