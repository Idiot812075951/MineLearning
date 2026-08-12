#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GunnerCharacter.generated.h"

class AMineableOre;
class UAnimSequenceBase;
class USceneComponent;
class UStaticMeshComponent;
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

	/** Requests exactly one shot. The AI owns the target; Gunner owns all shot details. */
	UFUNCTION(BlueprintCallable, Category="Gunner|Combat")
	bool TryFireAtOre(AMineableOre* TargetOre);

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category="Gunner|Combat")
	FVector GetMuzzleLocation() const;

	UPROPERTY(BlueprintAssignable, Category="Gunner|Combat")
	FGunnerShotResolvedSignature OnShotResolved;

	UPROPERTY(BlueprintAssignable, Category="Gunner|Combat")
	FGunnerReloadStateChangedSignature OnReloadStateChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	/** Editable child of the AK mesh. Place this at the barrel tip. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Weapon")
	TObjectPtr<USceneComponent> MuzzlePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gunner|Bark")
	TObjectPtr<UCompanionBarkComponent> BarkComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Animation")
	TObjectPtr<UAnimSequenceBase> FireAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Animation")
	FName FireSlotName = TEXT("DefaultSlot");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="1"))
	int32 MagazineSize = 30;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Gunner|Combat")
	int32 CurrentAmmo = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.01"))
	float FireInterval = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.01"))
	float ReloadDuration = 2.0f;

	/** Damage before the ore's existing hardness calculation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.0"))
	float BaseDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Combat", meta=(ClampMin="0.0"))
	float HeadshotDamageMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float HeadshotChance = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float GoldenHeadshotChance = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gunner|Accuracy", meta=(ClampMin="0.0"))
	float BodyShotChance = 0.7f;

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
	EGunnerShotResult RollShotResult() const;
	FVector CalculateShotTarget(const AMineableOre* TargetOre, EGunnerShotResult Result) const;
	void PlayFireAnimation();
	void PlayProductionShotVisual(EGunnerShotResult Result, const FVector& Start, const FVector& End) const;
	void SpawnHeadshotWorldFeedback(EGunnerShotResult Result, const FVector& TargetLocation);
	void DrawDefaultShotVisual(EGunnerShotResult Result, const FVector& Start, const FVector& End) const;
	void BeginReload();

	UFUNCTION()
	void CompleteReload();

	bool bIsReloading = false;
	double NextAllowedFireTime = 0.0;
	FTimerHandle ReloadTimerHandle;
};
