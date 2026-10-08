#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MineLearning/Combat/UnitEffectComponent.h"
#include "Engine/DataAsset.h"
#include "RunAbilityComponent.generated.h"

UCLASS(BlueprintType)
class MINELEARNING_API UOverclockDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FUnitEffectRule Boost;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FUnitEffectRule Overheat;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.01")) float Cooldown = 20.f;
	bool IsValidConfiguration() const;
};

UCLASS(BlueprintType)
class MINELEARNING_API UAIWorkDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FUnitEffectRule DisplayRule;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float BasePower = 0.25f;
	/** Additional to BasePower, not a second multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float FocusPower = 0.75f;
	bool IsValidConfiguration() const;
};

UENUM(BlueprintType)
enum class EOverclockPhase : uint8 { Ready, Boost, Overheat, Cooldown };

/** Player/run-owned state survives pawn swaps. Receives owned units from the run assembler. */
UCLASS(ClassGroup=(Run), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API URunAbilityComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URunAbilityComponent();
	void Configure(UOverclockDefinition* InOverclock, UAIWorkDefinition* InWork);
	void RegisterUnit(APawn* Unit);
	void SetControlledUnit(APawn* Unit);
	UFUNCTION(BlueprintCallable, Category="Run|Ability") bool ActivateOverclock();
	UFUNCTION(BlueprintCallable, Category="Run|Ability") bool CycleFocus();
	UFUNCTION(BlueprintPure, Category="Run|Ability") APawn* GetFocusUnit() const { return Focus.Get(); }
	UFUNCTION(BlueprintPure, Category="Run|Ability") FText GetAbilityStatus() const;
	UFUNCTION(BlueprintPure, Category="Run|Ability") EOverclockPhase GetPhase() const;
	UFUNCTION(BlueprintPure, Category="Run|Ability") float GetCooldownRemaining() const;
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnAbilityChanged;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	bool IsEligibleAI(APawn* Unit) const;
	UFUNCTION() void RefreshAI();
	UFUNCTION() void ControllerChanged(APawn* Unit, AController* OldController, AController* NewController);
	void RefreshOverclock();
	void PublishClock();
	UFUNCTION() void UnitDestroyed(AActor* Unit);
	UPROPERTY(Transient) TObjectPtr<UOverclockDefinition> Overclock;
	UPROPERTY(Transient) TObjectPtr<UAIWorkDefinition> Work;
	TArray<TWeakObjectPtr<APawn>> Units;
	TWeakObjectPtr<APawn> Controlled;
	TWeakObjectPtr<APawn> Focus;
	double ActivationTime = -1.;
	FTimerHandle AbilityTimer;
};
