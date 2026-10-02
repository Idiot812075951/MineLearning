#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "WeaponActionComponent.generated.h"

USTRUCT(BlueprintType)
struct FWeaponRuntimeState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Ammo = 0;
	UPROPERTY(BlueprintReadOnly) int32 Capacity = 20;
	UPROPERTY(BlueprintReadOnly) int32 MagazineId = 0;
	UPROPERTY(BlueprintReadOnly) bool bBurstEnabled = true;
	/** Effect-local counters, keyed by grant source, survive ordinary pawn swaps. */
	UPROPERTY() TMap<FName, int32> EffectCounters;
};

struct FWeaponShotContext
{
	float DamageMultiplier = 1.f;
	float ImpactScale = 1.f;
	bool bSpecial = false;
};

struct FReloadResultContext
{
	int32 BaseCapacity = 20;
	int32 Capacity = 20;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPrepareWeaponShot, FWeaponShotContext&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPrepareReloadResult, FReloadResultContext&);

/** Lower-level action state; effect hooks run BEFORE committing damage or magazine contents. */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UWeaponActionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWeaponActionComponent();
	FPrepareWeaponShot OnPrepareShot;
	FPrepareReloadResult OnPrepareReload;
	FSimpleMulticastDelegate OnShotCommitted;
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnWeaponStateChanged;
	FWeaponShotContext PrepareShot();
	void CommitShot();
	void CommitReload(int32 BaseCapacity);
	void SetPattern(FName Source, int32 Rounds);
	void RemovePattern(FName Source);
	int32 GetRoundsPerAttack() const;
	UFUNCTION(BlueprintPure, Category="Weapon") bool HasBurstPattern() const;
	UFUNCTION(BlueprintPure, Category="Weapon") bool IsBurstEnabled() const { return State.bBurstEnabled && HasBurstPattern(); }
	bool ToggleBurstMode();
	void Initialize(int32 Capacity);
	bool ConsumeRound();
	void Restore(const FWeaponRuntimeState& Saved);
	FWeaponRuntimeState Export() const { return State; }
	int32 GetCounter(FName Source) const;
	void SetCounter(FName Source, int32 Value);
	UFUNCTION(BlueprintPure, Category="Weapon") int32 GetCurrentAmmo() const { return State.Ammo; }
	UFUNCTION(BlueprintPure, Category="Weapon") int32 GetCapacity() const { return State.Capacity; }
	UFUNCTION(BlueprintPure, Category="Weapon") bool IsNextShotSpecial() const { return GetRemainingUntilSpecial() == 1; }
	UFUNCTION(BlueprintPure, Category="Weapon") int32 GetRemainingUntilSpecial() const;
	void SetSpecialPreview(FName Source, int32 Remaining);
	/** -1 uses authored chance. GM override does not edit assets or permanent progress. */
	float ReloadChanceOverride = -1.f;
private:
	UPROPERTY(Transient) FWeaponRuntimeState State;
	TMap<FName, int32> Patterns;
	TMap<FName, int32> SpecialPreviews;
};
