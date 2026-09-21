#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

USTRUCT(BlueprintType)
struct FCombatAttributes
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Strength = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Agility = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Intelligence (Energy)")) float Intelligence = 0.f;
};

USTRUCT(BlueprintType)
struct FSkillDamageSpec
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SkillId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Input;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float BaseDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float StrengthScale = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AgilityScale = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float IntelligenceScale = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bDealsDamage = true;
	float Evaluate(const FCombatAttributes& Attributes) const;
};

USTRUCT(BlueprintType)
struct FCombatDamageRequest
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite) TObjectPtr<AActor> Source = nullptr;
	UPROPERTY(BlueprintReadWrite) TObjectPtr<AActor> Target = nullptr;
	UPROPERTY(BlueprintReadWrite) FName SkillId;
	UPROPERTY(BlueprintReadWrite) float Multiplier = 1.f;
	// Ability-owned authorization: Q locks this at contact, Arrival uses pre-hit HP.
	UPROPERTY(BlueprintReadWrite) bool bExecute = false;
	UPROPERTY(BlueprintReadWrite) bool bNonLethal = false;
	UPROPERTY(BlueprintReadWrite) FVector HitLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite) FVector HitNormal = FVector::UpVector;
	UPROPERTY(BlueprintReadWrite) bool bPlayHitFeedback = true;
};

USTRUCT(BlueprintType)
struct FCombatDamageResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) bool bAccepted = false;
	UPROPERTY(BlueprintReadOnly) bool bExecuted = false;
	UPROPERTY(BlueprintReadOnly) float PreviousHealth = 0.f;
	UPROPERTY(BlueprintReadOnly) float CurrentHealth = 0.f;
	UPROPERTY(BlueprintReadOnly) float AppliedDamage = 0.f;
};

USTRUCT(BlueprintType)
struct FCombatSkillViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FSkillDamageSpec Spec;
	UPROPERTY(BlueprintReadOnly) FCombatAttributes Contributions;
	UPROPERTY(BlueprintReadOnly) float Damage = 0.f;
	UPROPERTY(BlueprintReadOnly) FText Description;
};

USTRUCT(BlueprintType)
struct FCombatPanelViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Name;
	UPROPERTY(BlueprintReadOnly) float Health = 0.f;
	UPROPERTY(BlueprintReadOnly) float MaxHealth = 0.f;
	UPROPERTY(BlueprintReadOnly) FCombatAttributes Attributes;
	UPROPERTY(BlueprintReadOnly) TArray<FCombatSkillViewData> Skills;
	UPROPERTY(BlueprintReadOnly) FText Details;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCombatStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatDamageResolved, const FCombatDamageRequest&, Request, const FCombatDamageResult&, Result);
