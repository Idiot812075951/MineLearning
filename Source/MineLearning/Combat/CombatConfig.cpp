#include "CombatConfig.h"

float FSkillDamageSpec::Evaluate(const FCombatAttributes& Attributes) const
{
	const float Value = BaseDamage + Attributes.Strength * StrengthScale
		+ Attributes.Agility * AgilityScale + Attributes.Intelligence * IntelligenceScale;
	return bDealsDamage && FMath::IsFinite(Value) ? FMath::Max(0.f, Value) : 0.f;
}

const FSkillDamageSpec* UCombatConfig::FindSkill(FName SkillId) const
{
	return Skills.FindByPredicate([SkillId](const FSkillDamageSpec& Skill) { return Skill.SkillId == SkillId; });
}
