#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.h"
#include "CombatModifiers.generated.h"

/** Additive bonuses. Percentages use fractions: 0.1 means +10%. */
USTRUCT(BlueprintType)
struct FCombatModifiers
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FCombatAttributes Attributes;
	/** Fractions applied after flat attribute bonuses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FCombatAttributes AttributePercent;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MoveSpeed = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AttackSpeed = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float CastSpeed = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float PrimaryDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float PrimaryDamageFlat = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AttackRange = 0.f;
	/** Scales both authored spray and random shot deviation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float BulletDrift = 0.f;
	/** Probability transferred from body shots, before burst accuracy reduction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float GoldenProbability = 0.f;
	/** Only AI attacks roll this. Player misses come from actual targeting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float AIMissChance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName WeaponStyle;
	bool IsValid() const;
	void Add(const FCombatModifiers& Other, float Scale = 1.f);
};
