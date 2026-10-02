#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatModifiers.h"
#include "UnitEffectComponent.generated.h"

class UMaterialInterface;
class UCombatComponent;
class UHealthComponent;
class UUnitEffectDefinition;
class UUnitEffectInstance;

UENUM(BlueprintType)
enum class EUnitEffectTrigger : uint8
{
	Persistent,
	Event
};

/** One reusable effect rule; no talent, summoner or reward concepts. */
USTRUCT(BlueprintType)
struct FUnitEffectRule
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) FName Id;
	UPROPERTY(EditAnywhere) FText DisplayName;
	UPROPERTY(EditAnywhere, Category="Presentation") TObjectPtr<UMaterialInterface> Icon;
	UPROPERTY(EditAnywhere, Category="Presentation", meta=(ClampMin="0", ClampMax="1")) float AuraIntensity = 0.f;
	UPROPERTY(EditAnywhere) EUnitEffectTrigger Trigger = EUnitEffectTrigger::Persistent;
	UPROPERTY(EditAnywhere) FName EventName;
	UPROPERTY(EditAnywhere) float Chance = 1.f;
	UPROPERTY(EditAnywhere) float Duration = 0.f;
	UPROPERTY(EditAnywhere) FCombatModifiers Modifiers;
	bool IsValid() const;
};

USTRUCT()
struct FActiveUnitEffect
{
	GENERATED_BODY()
	UPROPERTY() FUnitEffectRule Definition;
	/** Zero means persistent; otherwise game-world time, including time dilation and pause. */
	float EndTime = 0.f;
	int32 StackCount = 0;
};

USTRUCT(BlueprintType)
struct FUnitEffectView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FName Source;
	UPROPERTY(BlueprintReadOnly) FText Name;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UMaterialInterface> Icon;
	UPROPERTY(BlueprintReadOnly) FText StatusText;
	UPROPERTY(BlueprintReadOnly) float AuraIntensity = 0.f;
	UPROPERTY(BlueprintReadOnly) float Remaining = 0.f;
	UPROPERTY(BlueprintReadOnly) bool bTimed = false;
	UPROPERTY(BlueprintReadOnly) int32 StackCount = 0;
};

/** Persistent rules and refresh-only timed effects, owned by one unit. No tick. */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UUnitEffectComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUnitEffectComponent();
	bool GrantDefinition(FName Source, UUnitEffectDefinition* Definition);
	void RevokeDefinition(FName Source);
	TArray<FName> GetGrantedSources() const;
	void RefreshRuntimeState();
	void ClearEffects();
	void HandleEvent(FName EventName);
	/** Apply immediately, bypassing trigger chance. Reapplying the same ID refreshes it. */
	bool ApplyEffect(const FUnitEffectRule& Effect, int32 StackCount = 0);
	void RemoveEffect(FName EffectId);
	/** Removes active contributions but preserves the equipped trigger rules. */
	void RemoveAllEffects();
	float GetRemainingTime(FName RuleId) const;
	UFUNCTION(BlueprintPure, Category = "Combat") FText GetEffectSummary() const;
	UFUNCTION(BlueprintPure, Category="Effects") TArray<FUnitEffectView> GetActiveEffectViews() const;
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnEffectsChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UUnitEffectInstance>> Instances;
	UFUNCTION() void HealthChanged();
	void ScheduleNextChange();
	void TimeBoundaryReached();
	UPROPERTY(Transient) TObjectPtr<UCombatComponent> Combat;
	UPROPERTY(Transient) TObjectPtr<UHealthComponent> Health;
	UPROPERTY(Transient) TMap<FName, FActiveUnitEffect> ActiveEffects;
	FTimerHandle ChangeTimer;
};
