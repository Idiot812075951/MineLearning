#include "CombatModifiers.h"

bool FCombatModifiers::IsValid() const
{
	if (!FMath::IsFinite(AttributePercent.Strength) || !FMath::IsFinite(AttributePercent.Agility)
		|| !FMath::IsFinite(AttributePercent.Intelligence) || !FMath::IsFinite(PrimaryDamageFlat)
		|| !FMath::IsFinite(AttackRange) || !FMath::IsFinite(BulletDrift) || FMath::Abs(BulletDrift) > 100.f
		|| !FMath::IsFinite(AIMissChance) || AIMissChance < 0.f || AIMissChance > 1.f)
	{
		return false;
	}
	return FMath::IsFinite(Attributes.Strength) && FMath::IsFinite(Attributes.Agility) && FMath::IsFinite(Attributes.Intelligence) &&
	    FMath::IsFinite(MoveSpeed) && FMath::IsFinite(AttackSpeed) && FMath::IsFinite(CastSpeed) && FMath::IsFinite(PrimaryDamage) &&
	    FMath::IsFinite(GoldenProbability) && FMath::Abs(Attributes.Strength) <= 1000000.f && FMath::Abs(Attributes.Agility) <= 1000000.f &&
	    FMath::Abs(Attributes.Intelligence) <= 1000000.f &&
	    FMath::Abs(CastSpeed) <= 100.f && FMath::Abs(PrimaryDamage) <= 100.f && FMath::Abs(GoldenProbability) <= 1.f;
}

void FCombatModifiers::Add(const FCombatModifiers& Other, float Scale)
{
	Attributes.Strength += Other.Attributes.Strength * Scale;
	Attributes.Agility += Other.Attributes.Agility * Scale;
	Attributes.Intelligence += Other.Attributes.Intelligence * Scale;
	AttributePercent.Strength += Other.AttributePercent.Strength * Scale;
	AttributePercent.Agility += Other.AttributePercent.Agility * Scale;
	AttributePercent.Intelligence += Other.AttributePercent.Intelligence * Scale;
	MoveSpeed += Other.MoveSpeed * Scale;
	AttackSpeed += Other.AttackSpeed * Scale;
	CastSpeed += Other.CastSpeed * Scale;
	PrimaryDamage += Other.PrimaryDamage * Scale;
	PrimaryDamageFlat += Other.PrimaryDamageFlat * Scale;
	AttackRange += Other.AttackRange * Scale;
	BulletDrift += Other.BulletDrift * Scale;
	GoldenProbability += Other.GoldenProbability * Scale;
	AIMissChance += Other.AIMissChance * Scale;
	if (!Other.WeaponStyle.IsNone())
	{
		WeaponStyle = Other.WeaponStyle;
	}
}
