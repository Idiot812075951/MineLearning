#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatFeedbackComponent.generated.h"

class UCombatComponent;
class UUnitEffectComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Presentation subscriber. Never changes attacks, effect stacks or attributes. */
UCLASS(ClassGroup=(Presentation), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UCombatFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCombatFeedbackComponent();
	UPROPERTY(EditAnywhere, Category="Range") TSoftObjectPtr<UMaterialInterface> RangeMaterial;
	UPROPERTY(EditAnywhere, Category="Range", meta=(ClampMin="0.01")) float RangeDisplaySeconds = 2.f;
	UPROPERTY(EditAnywhere, Category="Aura") TSoftObjectPtr<UMaterialInterface> AuraMaterial;
	bool IsRangeVisible() const;
	float GetAuraIntensity() const { return AuraIntensity; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION() void ShowRange();
	UFUNCTION() void RefreshAura();
	void HideRange();
	void UpdateRange();
	UPROPERTY(Transient) TObjectPtr<UCombatComponent> Combat;
	UPROPERTY(Transient) TObjectPtr<UUnitEffectComponent> Effects;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> RangeRing;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> PreviousOverlay;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Aura;
	float AuraIntensity = 0.f;
	double RangeEndTime = 0.;
	FTimerHandle RangeTimer;
};
