#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MineLearning/TransformationGuard.h"
#include "AutonomousUnit.h"
#include "GunnerCharacter.generated.h"

class AMineableOre;
class APlayerController;
class UAnimInstance;
class UAnimMontage;
class UCameraComponent;
class UInputAction;
class UInputComponent;
class UInputMappingContext;
class USceneComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UAmmoInventoryComponent;
class UWeaponActionComponent;
class UWeaponRecoilComponent;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EGunnerShotResult : uint8
{
	BodyShot UMETA(DisplayName = "Body Shot"),
	Headshot UMETA(DisplayName = "Headshot"),
	GoldenHeadshot UMETA(DisplayName = "Golden Headshot"),
	Miss UMETA(DisplayName = "Miss"),
	OutOfRange UMETA(DisplayName = "Out of Range")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FGunnerShotResolvedSignature,
	EGunnerShotResult, Result,
	FVector, MuzzleLocation,
	FVector, TargetLocation,
	float, AppliedDamage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGunnerReloadStateChangedSignature, bool, bReloading);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGunnerCriticalHitSignature, bool, bGolden, AActor*, Target, FVector, TargetTop);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGunnerWeaponFiredSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGunnerAmmoChangedSignature,
	int32, CurrentAmmo,
	int32, MagazineSize);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FGunnerControlModeChangedSignature,
	bool, bPlayerControlled);

UCLASS(BlueprintType)
class MINELEARNING_API AGunnerCharacter : public ACharacter, public ITransformationGuard, public IAutonomousUnit
{
	GENERATED_BODY()

public:
	AGunnerCharacter();
	virtual bool SupportsAutonomousControl() const override { return AIControllerClass != nullptr; }
	virtual EAutonomousOutput GetAutonomousOutput() const override { return EAutonomousOutput::PrimaryDamage; }
	virtual bool CanTransform() const override;
	UFUNCTION(BlueprintPure, Category = "Combat") float GetShotMultiplier(EGunnerShotResult Result) const;
	UFUNCTION(BlueprintPure, Category = "Combat") FText GetCombatMechanics() const;
	UFUNCTION(BlueprintPure, Category="Gunner|Combat") FText GetAmmoStatusText() const;
	bool IsWeaponBusy() const { return bIsReloading || bReloadPending || bBurstInProgress; }
	/** Normalized body/head/golden/miss probabilities; reading never rolls randomness. */
	FVector4 GetShotProbabilities(bool bBurst) const;
	void RestoreLoadedAmmo(int32 Ammo);
	void SetAmmoAccount(UAmmoInventoryComponent* Account);
	/** Equip an empty weapon from reserves without a reload animation during transformation. */
	bool TryLoadEmptyMagazine();
	void CancelPlayerFire();
	UFUNCTION(BlueprintCallable, Category="Gunner|Combat") void ToggleFireMode();
	UFUNCTION(BlueprintPure, Category="Gunner|Combat") bool HasBurstMode() const;
	UFUNCTION(BlueprintPure, Category="Gunner|Combat") bool IsBurstMode() const;
	float GetEffectiveAttackRange() const;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** AI request: chooses the existing point-shot or three-round burst behavior. */
	UFUNCTION(BlueprintCallable, Category="Gunner|Combat")
	bool TryFireAtOre(AMineableOre* TargetOre);

	/** Fires a player burst through the crosshair. An ore target is optional. */
	UFUNCTION(BlueprintCallable, Category="Gunner|Combat")
	bool TryFireAtAim(FVector AimOrigin, FVector AimDirection);

	/** Starts the same reload transaction used by the AI when the magazine is not full. */
	UFUNCTION(BlueprintCallable, Category="Gunner|Combat")
	bool RequestReload();

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	int32 GetCurrentAmmo() const;

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	int32 GetMagazineSize() const;

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	FVector GetMuzzleLocation() const;

	UPROPERTY(BlueprintAssignable, Category="Gunner|Combat")
	FGunnerShotResolvedSignature OnShotResolved;
	/** A critical hit that actually dealt damage, excluding misses and invalid targets. */
	UPROPERTY(BlueprintAssignable, Category="Gunner|Combat")
	FGunnerCriticalHitSignature OnCriticalHit;

	UPROPERTY(BlueprintAssignable, Category="Gunner|Combat")
	FGunnerReloadStateChangedSignature OnReloadStateChanged;

	/** Fired once for every projectile actually emitted, including each burst round. */
	UPROPERTY(BlueprintAssignable, Category="Gunner|Combat")
	FGunnerWeaponFiredSignature OnWeaponFired;

	/** Authoritative magazine snapshot after ammo actually changes. */
	UPROPERTY(BlueprintAssignable, Category="Gunner|Combat")
	FGunnerAmmoChangedSignature OnAmmoChanged;

	/** Announces possession-mode changes without referencing any presentation object. */
	UPROPERTY(BlueprintAssignable, Category="Gunner|Control")
	FGunnerControlModeChangedSignature OnControlModeChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	/** Independent magazine. It shares the weapon origin while inserted. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Weapon")
	TObjectPtr<UStaticMeshComponent> MagazineMesh;

	/** Editable child of the AK mesh. Place this at the barrel tip. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Weapon")
	TObjectPtr<USceneComponent> MuzzlePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Player Control")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Player Control")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Weapon")
	TObjectPtr<UWeaponRecoilComponent> Recoil;

	UPROPERTY()
	TObjectPtr<UInputAction> SecondarySkillAction;

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
	int32 MagazineSize = 20;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Gunner|Combat")
	TObjectPtr<UWeaponActionComponent> WeaponActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.01"))
	float FireInterval = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="1"))
	float BurstFireRateMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.01"))
	float ReloadDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.0"))
	float HeadshotDamageMultiplier = 1.8f;
	UPROPERTY(EditAnywhere, Category="Gunner|Combat") float GoldenHeadshotMultiplier = 2.5f;
	UPROPERTY(EditAnywhere, Category="Gunner|Combat") float HeadshotIntelligenceScale = 0.001f;
	UPROPERTY(EditAnywhere, Category="Gunner|Combat") float GoldenIntelligenceScale = 0.002f;
	UPROPERTY(EditAnywhere, Category="Gunner|Combat", meta=(ClampMin="1")) float HeadshotMultiplierMax = 2.2f;
	UPROPERTY(EditAnywhere, Category="Gunner|Combat", meta=(ClampMin="1")) float GoldenMultiplierMax = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float HeadshotChance = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float GoldenHeadshotChance = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float HitJitterFraction = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="1.0"))
	float MissRadiusMultiplier = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player Control|Combat", meta=(ClampMin="100.0"))
	float PlayerAimRange = 5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player Control|Aim")
	FVector2D PlayerWeaponPitchRange = FVector2D(-45.0f, 35.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player Control|Aim", meta=(ClampMin="0.0"))
	float PlayerWeaponAimInterpSpeed = 14.0f;

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
	float ActiveFirePlayRate = 1.f;
	float ActiveReloadSpeed = 1.f;
	float ActiveHeadMultiplier = 1.f;
	float ActiveGoldenMultiplier = 1.f;
	FVector4 ActiveShotProbabilities = FVector4(1.f, 0.f, 0.f, 0.f);
	UAmmoInventoryComponent* GetAmmoAccount() const;
	UFUNCTION() void NotifyAmmoChanged();
	struct FShotTarget
	{
		TWeakObjectPtr<AActor> Actor;
		FVector AimLocation = FVector::ZeroVector;
		bool bUseExactAimLocation = false;
		FVector AimOrigin = FVector::ZeroVector;
		FVector AimDirection = FVector::ForwardVector;
	};

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartPlayerFire();
	void StopPlayerFire();
	void StopPlayerAim();
	void StartPlayerReload();
	void UpdatePlayerAim(float DeltaSeconds);
	void TryResolvePendingPlayerShot();
	void ConfigureControllerMode();
	void ApplyLocalPlayerViewport();
	bool TryStartAttack(const FShotTarget& Target);
	EGunnerShotResult RollShotResult(bool bUseBurstAccuracy) const;
	FVector CalculateShotTarget(const FShotTarget& Target, EGunnerShotResult Result) const;
	float PlayFireMontage();
	bool PlayBurstFireMontage();
	void ResolveShot(const FShotTarget& Target, bool bUseBurstAccuracy, int32 BurstRoundIndex = 0);
	void EndBurst(const TCHAR* Reason);
	void ResolveBurstRound(const TCHAR* Trigger);
	void ResolveBurstTimedShot();
	void PlayProductionShotVisual(EGunnerShotResult Result, const FVector& Start, const FVector& End, float ImpactScale = 1.f) const;
	void DrawDefaultShotVisual(EGunnerShotResult Result, const FVector& Start, const FVector& End, float ImpactScale = 1.f) const;
	void BeginReload();
	void QueueReloadAfterSingleShot(float ShotMontageDuration);
	bool PlayReloadMontage();
	void HandleReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void ForceCompleteReload();
	void AttachMagazineToHand();
	void AttachMagazineToWeapon();
	void RegisterReloadNotifyHandlers();
	void UnregisterReloadNotifyHandlers();

	UFUNCTION()
	void AnimNotify_Mag_ToHand();

	UFUNCTION()
	void AnimNotify_Mag_ToGun();

	UFUNCTION()
	void CompleteReload();

	UPROPERTY(Transient) TObjectPtr<UAmmoInventoryComponent> AmmoAccount;
	void ContinuePlayerFire();
	void CancelReload();
	FTimerHandle ContinuousFireTimer;
	bool bIsReloading = false;
	bool bReloadPending = false;
	bool bBurstInProgress = false;
	int32 BurstRoundsResolved = 0;
	FShotTarget BurstTarget;
	FRotator WeaponBaseRelativeRotation = FRotator::ZeroRotator;
	float CurrentWeaponAimPitch = 0.0f;
	bool bPlayerShotPending = false;
	bool bPlayerAimInputHeld = false;
	double NextAllowedFireTime = 0.0;
	FTimerHandle ReloadTimerHandle;
	FTimerHandle PendingReloadTimerHandle;
	FTimerHandle BurstRoundTimerHandle;
	TWeakObjectPtr<UAnimInstance> ReloadNotifyAnimInstance;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveReloadMontage;
};
