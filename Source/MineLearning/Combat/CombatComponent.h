#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatConfig.h"
#include "CombatModifiers.h"
#include "CombatComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FPrimaryAttackResolved, bool /*bHit*/, bool /*bMoving*/);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCombatComponent();
	bool SetModifier(FName Source, const FCombatModifiers& Modifier);
	void RemoveModifier(FName Source);
	UFUNCTION(BlueprintPure, Category="Combat") FCombatModifiers GetModifiers() const { return EffectiveModifiers; }
	float GetMoveSpeedScale() const { return FMath::Max(0.1f, 1.f + EffectiveModifiers.MoveSpeed); }
	float GetAttackSpeedScale() const { return FMath::Max(0.1f, 1.f + EffectiveModifiers.AttackSpeed); }
	float GetCastSpeedScale() const { return FMath::Clamp(1.f + EffectiveModifiers.CastSpeed, 0.1f, 10.f); }
	float GetAttackRangeScale() const { return FMath::Clamp(1.f + EffectiveModifiers.AttackRange, 0.1f, 10.f); }
	float EvaluateDamage(FName SkillId) const;
	float GetAttackRange() const;
	float GetAttackInterval(float BaseInterval) const;
	float GetAttackPlayRate(float BaseHitInterval) const;
	float GetMaxMoveSpeed() const;
	/** Distance to the target collision surface, not its center; shared by AI and attacks. */
	float GetAttackDistance(const AActor* Target) const;
	bool IsInAttackRange(const AActor* Target) const;
	void NotifyAttackResolved(bool bHit, bool bCheckAimOnMiss = true);
	bool IsStrafing() const;
	void NotifyAttackOutOfRange();
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnAttackOutOfRange;
	bool RollAIAttackMiss() const;
	FPrimaryAttackResolved OnPrimaryAttackResolved;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat") TSoftObjectPtr<UCombatConfig> Config;
	UCombatConfig* GetConfig() const { return Config.LoadSynchronous(); }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat") FName PrimarySkillId = TEXT("Primary");
	UFUNCTION(BlueprintPure) FCombatAttributes GetAttributes() const;
	UFUNCTION(BlueprintPure) FCombatPanelViewData GetPanelData() const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly) void SetAttributeBonus(FCombatAttributes Bonus);
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnAttributesChanged;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
private:
	TMap<FName, FCombatModifiers> Modifiers;
	UPROPERTY(ReplicatedUsing=OnRep_Bonus) FCombatModifiers EffectiveModifiers;
	void RecomputeModifiers();
	UPROPERTY(ReplicatedUsing = OnRep_Bonus) FCombatAttributes AttributeBonus;
	UFUNCTION() void OnRep_Bonus();
};
