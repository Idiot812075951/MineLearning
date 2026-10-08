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
class UPointLightComponent;
class USoundBase;

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
	UPROPERTY(EditAnywhere, Category="Aura") TSoftObjectPtr<UMaterialInterface> WorkRingMaterial;
	UPROPERTY(EditAnywhere, Category="Aura") TSoftObjectPtr<UMaterialInterface> WorkMarkerMaterial;
	UPROPERTY(EditAnywhere, Category="Aura") TSoftObjectPtr<USoundBase> ChargeReadySound;
	UPROPERTY(EditAnywhere, Category="Aura") TSoftObjectPtr<UMaterialInterface> SteamMaterial;
	UPROPERTY(EditAnywhere, Category="Aura") FName ToolGlowSocket = TEXT("S_DrillTip");
	UPROPERTY(EditAnywhere, Category="Aura") FName ToolMaterialSlot = TEXT("MAT_OB07_ToolMetal");
	bool IsRangeVisible() const;
	float GetAuraIntensity() const { return AuraIntensity; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION() void ShowRange();
	UFUNCTION() void AttributesChanged();
	UFUNCTION() void RefreshAura();
	void HideRange();
	void UpdateRange();
	void RefreshWorkIndicators(float Intensity, bool bMarker, const FLinearColor& Color);
	void RefreshToolGlow(bool bEnabled, const FLinearColor& Color);
	void RefreshSteam(bool bEnabled);
	UPROPERTY(Transient) TObjectPtr<UCombatComponent> Combat;
	UPROPERTY(Transient) TObjectPtr<UUnitEffectComponent> Effects;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> RangeRing;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> PreviousOverlay;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Aura;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> WorkRing;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> WorkMarker;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> WorkRingMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> WorkMarkerMID;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> ToolLight;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ToolMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> SteamPlanes;
	TMap<FName, TWeakObjectPtr<USoundBase>> ActiveSounds;
	bool bToolWasGlowing = false;
	float AuraIntensity = 0.f;
	float ObservedAttackRange = 0.f;
	FVector OriginalMeshScale = FVector::OneVector;
	double RangeEndTime = 0.;
	FTimerHandle RangeTimer;
};
