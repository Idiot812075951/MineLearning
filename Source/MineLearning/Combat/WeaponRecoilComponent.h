#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "WeaponRecoilComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWeaponRecoilChanged);

/** Shot geometry and recovery only. Input, damage and crosshair rendering remain outside. */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UWeaponRecoilComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWeaponRecoilComponent();
	FVector ApplyShot(FVector Direction);
	UFUNCTION(BlueprintPure, Category="Weapon") FVector2D GetCrosshairScale() const;
	UPROPERTY(BlueprintAssignable) FWeaponRecoilChanged OnRecoilChanged;
	/** X = yaw, Y = upward pitch, in degrees. Last point repeats during sustained fire. */
	UPROPERTY(EditAnywhere, Category="Recoil") TArray<FVector2D> SprayPattern;
	UPROPERTY(EditAnywhere, Category="Recoil", meta=(ClampMin="0")) float RandomSpreadDegrees = 0.3f;
	UPROPERTY(EditAnywhere, Category="Recoil", meta=(ClampMin="0")) float RecoveryDelay = 0.85f;
	UPROPERTY(EditAnywhere, Category="Recoil", meta=(ClampMin="0.1")) float RecoveryPerSecond = 12.f;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void Recover();
	float Heat = 0.f;
	double LastShotTime = 0.;
	double LastRecoveryTime = 0.;
	FTimerHandle RecoveryTimer;
};
