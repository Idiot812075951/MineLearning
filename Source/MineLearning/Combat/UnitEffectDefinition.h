#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UnitEffectComponent.h"
#include "WeaponActionComponent.h"
#include "UnitEffectDefinition.generated.h"

class UUnitEffectInstance;

UCLASS(BlueprintType)
class MINELEARNING_API UUnitEffectDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UUnitEffectDefinition();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") FUnitEffectRule Rule;
	/** Optional restriction, also enforced for GM/direct grants. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") TSoftClassPtr<AActor> RequiredTargetClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") TSubclassOf<UUnitEffectInstance> InstanceClass;
	virtual bool ValidateDefinition(FString& Error) const;
	virtual bool SupportsTarget(const AActor* Target) const;
	virtual FText GetRuleDescription() const { return FText::GetEmpty(); }
};

UCLASS(BlueprintType)
class MINELEARNING_API UShotSequenceEffectDefinition : public UUnitEffectDefinition
{
	GENERATED_BODY()
public:
	UShotSequenceEffectDefinition();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shot", meta=(ClampMin="1")) int32 EveryN = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shot", meta=(ClampMin="0")) float DamageMultiplier = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shot", meta=(ClampMin="0")) float ImpactScale = 2.f;
	virtual bool ValidateDefinition(FString& Error) const override;
	virtual bool SupportsTarget(const AActor* Target) const override;
};

UCLASS(BlueprintType)
class MINELEARNING_API UReloadCapacityEffectDefinition : public UUnitEffectDefinition
{
	GENERATED_BODY()
public:
	UReloadCapacityEffectDefinition();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload", meta=(ClampMin="0", ClampMax="1")) float Chance = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload", meta=(ClampMin="1", ClampMax="10")) int32 CapacityMultiplier = 2;
	virtual bool ValidateDefinition(FString& Error) const override;
	virtual bool SupportsTarget(const AActor* Target) const override;
};

UCLASS(BlueprintType)
class MINELEARNING_API UAttackPatternEffectDefinition : public UUnitEffectDefinition
{
	GENERATED_BODY()
public:
	UAttackPatternEffectDefinition();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="1", ClampMax="3")) int32 RoundsPerAttack = 3;
	virtual bool ValidateDefinition(FString& Error) const override;
	virtual bool SupportsTarget(const AActor* Target) const override;
};

UCLASS()
class MINELEARNING_API UUnitEffectInstance : public UObject
{
	GENERATED_BODY()
public:
	virtual bool Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source);
	virtual void Revoke();
	virtual void HandleEvent(FName EventName);
	virtual void RefreshState() {}
	/** True if this instance handled expiry (for example, by decaying one stack). */
	virtual bool HandleExpiry() { return false; }
protected:
	UPROPERTY(Transient) TObjectPtr<UUnitEffectComponent> Effects;
	UPROPERTY(Transient) TObjectPtr<UUnitEffectDefinition> Definition;
	FName GrantSource;
};

UCLASS()
class MINELEARNING_API UShotSequenceEffectInstance : public UUnitEffectInstance
{
	GENERATED_BODY()
public:
	virtual bool Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source) override;
	virtual void Revoke() override;
	virtual void RefreshState() override;
private:
	void Prepare(FWeaponShotContext& Context);
	void Committed();
	UPROPERTY(Transient) TObjectPtr<UWeaponActionComponent> Weapon;
	FDelegateHandle PrepareHandle;
	FDelegateHandle CommittedHandle;
};

UCLASS()
class MINELEARNING_API UReloadCapacityEffectInstance : public UUnitEffectInstance
{
	GENERATED_BODY()
public:
	virtual bool Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source) override;
	virtual void Revoke() override;
private:
	void Prepare(FReloadResultContext& Context);
	UPROPERTY(Transient) TObjectPtr<UWeaponActionComponent> Weapon;
	FDelegateHandle PrepareHandle;
};

UCLASS()
class MINELEARNING_API UAttackPatternEffectInstance : public UUnitEffectInstance
{
	GENERATED_BODY()
public:
	virtual bool Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source) override;
	virtual void Revoke() override;
private:
	UPROPERTY(Transient) TObjectPtr<UWeaponActionComponent> Weapon;
};
