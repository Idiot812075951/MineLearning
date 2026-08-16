#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GunnerCharacter.generated.h"

class AMineableOre;
class UAnimMontage;
class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UNiagaraSystem;
class UCompanionBarkComponent;

UENUM(BlueprintType)
enum class EGunnerShotResult : uint8
{
	BodyShot UMETA(DisplayName = "Body Shot"),
	Headshot UMETA(DisplayName = "Headshot"),
	GoldenHeadshot UMETA(DisplayName = "Golden Headshot"),
	Miss UMETA(DisplayName = "Miss")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FGunnerShotResolvedSignature,
	EGunnerShotResult, Result,
	FVector, MuzzleLocation,
	FVector, TargetLocation,
	float, AppliedDamage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGunnerReloadStateChangedSignature, bool, bReloading);

UCLASS(BlueprintType)
class MINELEARNING_API AGunnerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGunnerCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Requests a point shot or a three-round burst, chosen per attack. */
	UFUNCTION(BlueprintCallable, Category="Gunner|Combat")
	bool TryFireAtOre(AMineableOre* TargetOre);

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	int32 GetMagazineSize() const { return MagazineSize; }

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	FVector GetMuzzleLocation() const;

	UPROPERTY(BlueprintAssignable, Category="Gunner|Combat")
	FGunnerShotResolvedSignature OnShotResolved;

	UPROPERTY(BlueprintAssignable, Category="Gunner|Combat")
	FGunnerReloadStateChangedSignature OnReloadStateChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	/** Independent magazine. It shares the weapon origin while inserted. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Weapon")
	TObjectPtr<UStaticMeshComponent> MagazineMesh;

	/** Editable child of the AK mesh. Place this at the barrel tip. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Weapon")
	TObjectPtr<USceneComponent> MuzzlePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Bark")
	TObjectPtr<UCompanionBarkComponent> BarkComponent;

	/** Persistent read-only ammo display. Weapon state remains owned by Gunner. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|UI")
	TObjectPtr<UWidgetComponent> AmmoWidgetComponent;

	/** Persistent Montage assets. Do not replace these with transient dynamic montages. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	/** Three-round fire Montage. Gameplay resolves its rounds at the matching animation frames. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Animation")
	TObjectPtr<UAnimMontage> BurstFireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Weapon")
	FName MagazineHandSocketName = TEXT("Socket_Magazine_L");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="1"))
	int32 MagazineSize = 10;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Gunner|Combat")
	int32 CurrentAmmo = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.01"))
	float FireInterval = 0.7f;

	/** Burst chance when at least three rounds remain; otherwise Gunner always point-fires. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.0", ClampMax="1.0"))
	float BurstChance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.01"))
	float ReloadDuration = 2.0f;

	/** Guarantees recovery if an AnimBP/slot interrupts a reload Montage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.0"))
	float MontageSafetyPadding = 0.25f;

	/** Damage before the ore's existing hardness calculation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.0"))
	float BaseDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.0"))
	float HeadshotDamageMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float HeadshotChance = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float GoldenHeadshotChance = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float BodyShotChance = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float MissChance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float HitJitterFraction = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="1.0"))
	float MissRadiusMultiplier = 1.35f;

	/** Optional debug-only line. Production shots use a collision-free Niagara projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Visual")
	bool bDrawShotDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Visual", meta=(ClampMin="0.01"))
	float TracerLifeSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Visual", meta=(ClampMin="0.1"))
	float TracerThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Visual")
	TObjectPtr<UNiagaraSystem> TracerSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Visual")
	TObjectPtr<UNiagaraSystem> MuzzleFlashSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Visual")
	TObjectPtr<UNiagaraSystem> HeadshotSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Visual")
	TObjectPtr<UNiagaraSystem> GoldenHeadshotSystem;

	/** Optional BP hook for production muzzle/projectile/impact effects. Never performs hit tests. */
	UFUNCTION(BlueprintImplementableEvent, Category="Gunner|Visual")
	void PlayShotVisuals(EGunnerShotResult Result, FVector MuzzleLocation, FVector TargetLocation);

private:
	EGunnerShotResult RollShotResult(bool bUseBurstAccuracy) const;
	FVector CalculateShotTarget(const AMineableOre* TargetOre, EGunnerShotResult Result) const;
	void PlayFireMontage();
	bool PlayBurstFireMontage();
	void ResolveShot(AMineableOre* TargetOre, bool bUseBurstAccuracy, int32 BurstRoundIndex = 0);
	void EndBurst(const TCHAR* Reason);
	void ResolveBurstRound(const TCHAR* Trigger);
	void ResolveBurstTimedShot();
	void ResolveOutstandingBurstShots();
	void PlayProductionShotVisual(EGunnerShotResult Result, const FVector& Start, const FVector& End) const;
	void SpawnHeadshotWorldFeedback(EGunnerShotResult Result, const FVector& TargetLocation);
	void DrawDefaultShotVisual(EGunnerShotResult Result, const FVector& Start, const FVector& End) const;
	void BeginReload();
	bool PlayReloadMontage();
	void HandleReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void ForceCompleteReload();
	void AttachMagazineToHand();
	void AttachMagazineToWeapon();
	void UpdateAmmoDisplay();
	void RegisterReloadNotifyHandlers();
	void UnregisterReloadNotifyHandlers();
#if WITH_EDITOR
	void EnsureBurstMontageNotifies();
#endif

	UFUNCTION()
	void AnimNotify_Mag_ToHand();

	UFUNCTION()
	void AnimNotify_Mag_ToGun();

	/** Called by each GunnerBurstShot notify on the three-round animation. */
	UFUNCTION()
	void AnimNotify_GunnerBurstShot();

	UFUNCTION()
	void CompleteReload();

	bool bIsReloading = false;
	bool bBurstInProgress = false;
	/** The Montage has no reliable persisted notify track, so bursts are timed from its 24 fps source frames. */
	bool bBurstTimingDriven = false;
	int32 BurstRoundsResolved = 0;
	TWeakObjectPtr<AMineableOre> BurstTargetOre;
	double NextAllowedFireTime = 0.0;
	FTimerHandle ReloadTimerHandle;
	FTimerHandle BurstRoundTimerHandle;
	FTimerHandle BurstSafetyTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveReloadMontage;
};
