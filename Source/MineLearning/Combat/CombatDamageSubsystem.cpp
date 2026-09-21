#include "CombatDamageSubsystem.h"
#include "CombatComponent.h"
#include "HealthComponent.h"
#include "GameFramework/Actor.h"

bool UCombatDamageSubsystem::CanDamageTarget(const AActor* Source, const AActor* Target)
{
	if (!IsValid(Source) || !IsValid(Target) || Source == Target || Source->GetWorld() != Target->GetWorld()
		|| Source->IsActorBeingDestroyed() || Target->IsActorBeingDestroyed() || !Target->CanBeDamaged()) { return false; }
	const UHealthComponent* From = Source->FindComponentByClass<UHealthComponent>();
	const UHealthComponent* To = Target->FindComponentByClass<UHealthComponent>();
	return From && To && !From->IsDead() && !To->IsDead()
		&& (To->Faction == ECombatFaction::Resource || From->Faction != To->Faction);
}

FCombatDamageResult UCombatDamageSubsystem::ApplyDamage(const FCombatDamageRequest& Request)
{
	if (!CanDamageTarget(Request.Source, Request.Target) || Request.Source->GetWorld() != GetWorld()
		|| !Request.Source->HasAuthority() || !Request.Target->HasAuthority()
		|| !FMath::IsFinite(Request.Multiplier) || Request.Multiplier < 0.f) { return {}; }
	const UCombatComponent* Combat = Request.Source->FindComponentByClass<UCombatComponent>();
	const UCombatConfig* Definition = Combat ? Combat->GetConfig() : nullptr;
	const FSkillDamageSpec* Skill = Definition ? Definition->FindSkill(Request.SkillId) : nullptr;
	if (!Skill || !Skill->bDealsDamage) { return {}; }
	const float Damage = Skill->Evaluate(Combat->GetAttributes()) * Request.Multiplier;
	if (!FMath::IsFinite(Damage)) { return {}; }
	return Request.Target->FindComponentByClass<UHealthComponent>()->ApplyResolvedDamage(Request, Damage);
}

FCombatDamageResult UCombatDamageSubsystem::ApplyDebugDamage(AActor* Target, float Amount)
{
#if !UE_BUILD_SHIPPING
	if (IsValid(Target) && Target->GetWorld() == GetWorld() && Target->HasAuthority() && Target->CanBeDamaged()
		&& !Target->IsActorBeingDestroyed() && FMath::IsFinite(Amount) && Amount > 0.f)
	{
		if (UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>(); Health && !Health->IsDead())
		{
			FCombatDamageRequest Request;
			Request.Target = Target;
			Request.SkillId = TEXT("GM");
			Request.HitLocation = Target->GetActorLocation();
			return Health->ApplyResolvedDamage(Request, Amount);
		}
	}
#endif
	return {};
}
