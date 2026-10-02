#include "UnitEffectDefinition.h"
#include "CombatComponent.h"

UUnitEffectDefinition::UUnitEffectDefinition()
{
	InstanceClass = UUnitEffectInstance::StaticClass();
}

bool UUnitEffectDefinition::ValidateDefinition(FString& Error) const
{
	if (!Rule.IsValid() || !InstanceClass || InstanceClass->HasAnyClassFlags(CLASS_Abstract))
	{
		Error = TEXT("Invalid effect rule or runtime class: ") + GetName();
		return false;
	}
	return true;
}

bool UUnitEffectDefinition::SupportsTarget(const AActor* Target) const
{
	return Target && Target->FindComponentByClass<UCombatComponent>()
		&& (RequiredTargetClass.IsNull() || Target->IsA(RequiredTargetClass.LoadSynchronous()));
}

bool UShotSequenceEffectDefinition::SupportsTarget(const AActor* Target) const
{
	return Super::SupportsTarget(Target) && Target->FindComponentByClass<UWeaponActionComponent>();
}

bool UReloadCapacityEffectDefinition::SupportsTarget(const AActor* Target) const
{
	return Super::SupportsTarget(Target) && Target->FindComponentByClass<UWeaponActionComponent>();
}

bool UAttackPatternEffectDefinition::SupportsTarget(const AActor* Target) const
{
	return Super::SupportsTarget(Target) && Target->FindComponentByClass<UWeaponActionComponent>();
}

bool UAttackPatternEffectDefinition::ValidateDefinition(FString& Error) const
{
	return Super::ValidateDefinition(Error) && InstanceClass->IsChildOf(UAttackPatternEffectInstance::StaticClass())
		&& RoundsPerAttack >= 1 && RoundsPerAttack <= 3;
}

UShotSequenceEffectDefinition::UShotSequenceEffectDefinition()
{
	InstanceClass = UShotSequenceEffectInstance::StaticClass();
}

bool UShotSequenceEffectDefinition::ValidateDefinition(FString& Error) const
{
	return UUnitEffectDefinition::ValidateDefinition(Error) && InstanceClass->IsChildOf(UShotSequenceEffectInstance::StaticClass())
		&& EveryN >= 1 && EveryN <= 1000000
		&& FMath::IsFinite(DamageMultiplier) && DamageMultiplier >= 0.f
		&& FMath::IsFinite(ImpactScale) && ImpactScale >= 0.f;
}

UReloadCapacityEffectDefinition::UReloadCapacityEffectDefinition()
{
	InstanceClass = UReloadCapacityEffectInstance::StaticClass();
}

bool UReloadCapacityEffectDefinition::ValidateDefinition(FString& Error) const
{
	return UUnitEffectDefinition::ValidateDefinition(Error) && InstanceClass->IsChildOf(UReloadCapacityEffectInstance::StaticClass())
		&& FMath::IsFinite(Chance)
		&& Chance >= 0.f && Chance <= 1.f && CapacityMultiplier >= 1 && CapacityMultiplier <= 10;
}

UAttackPatternEffectDefinition::UAttackPatternEffectDefinition()
{
	InstanceClass = UAttackPatternEffectInstance::StaticClass();
}

bool UUnitEffectInstance::Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source)
{
	Effects = Target;
	Definition = Config;
	GrantSource = Source;
	if (Config->Rule.Trigger == EUnitEffectTrigger::Persistent)
	{
		FUnitEffectRule Applied = Config->Rule;
		Applied.Id = GrantSource;
		return Effects->ApplyEffect(Applied);
	}
	return true;
}

void UUnitEffectInstance::Revoke()
{
	if (Effects)
	{
		Effects->RemoveEffect(GrantSource);
	}
}

void UUnitEffectInstance::HandleEvent(FName EventName)
{
	const FUnitEffectRule& Rule = Definition->Rule;
	if (Rule.Trigger == EUnitEffectTrigger::Event && Rule.EventName == EventName && FMath::FRand() < Rule.Chance)
	{
		FUnitEffectRule Applied = Rule;
		Applied.Id = GrantSource;
		Effects->ApplyEffect(Applied);
	}
}

bool UShotSequenceEffectInstance::Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source)
{
	Weapon = Target->GetOwner()->FindComponentByClass<UWeaponActionComponent>();
	if (!Weapon || !Cast<UShotSequenceEffectDefinition>(Config) || !Super::Grant(Target, Config, Source))
	{
		return false;
	}
	PrepareHandle = Weapon->OnPrepareShot.AddUObject(this, &UShotSequenceEffectInstance::Prepare);
	CommittedHandle = Weapon->OnShotCommitted.AddUObject(this, &UShotSequenceEffectInstance::Committed);
	RefreshState();
	return true;
}

void UShotSequenceEffectInstance::Prepare(FWeaponShotContext& Context)
{
	const UShotSequenceEffectDefinition* Config = CastChecked<UShotSequenceEffectDefinition>(Definition);
	if ((Weapon->GetCounter(GrantSource) + 1) % Config->EveryN == 0)
	{
		Context.DamageMultiplier *= Config->DamageMultiplier;
		Context.ImpactScale *= Config->ImpactScale;
		Context.bSpecial = true;
	}
}

void UShotSequenceEffectInstance::Committed()
{
	const UShotSequenceEffectDefinition* Config = CastChecked<UShotSequenceEffectDefinition>(Definition);
	Weapon->SetCounter(GrantSource, (Weapon->GetCounter(GrantSource) + 1) % Config->EveryN);
	RefreshState();
}

void UShotSequenceEffectInstance::RefreshState()
{
	const UShotSequenceEffectDefinition* Config = CastChecked<UShotSequenceEffectDefinition>(Definition);
	Weapon->SetSpecialPreview(GrantSource, Config->EveryN - Weapon->GetCounter(GrantSource) % Config->EveryN);
}

void UShotSequenceEffectInstance::Revoke()
{
	if (Weapon)
	{
		Weapon->OnPrepareShot.Remove(PrepareHandle);
		Weapon->OnShotCommitted.Remove(CommittedHandle);
		Weapon->SetSpecialPreview(GrantSource, 0);
	}
	Super::Revoke();
}

bool UReloadCapacityEffectInstance::Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source)
{
	Weapon = Target->GetOwner()->FindComponentByClass<UWeaponActionComponent>();
	if (!Weapon || !Cast<UReloadCapacityEffectDefinition>(Config) || !Super::Grant(Target, Config, Source))
	{
		return false;
	}
	PrepareHandle = Weapon->OnPrepareReload.AddUObject(this, &UReloadCapacityEffectInstance::Prepare);
	return true;
}

void UReloadCapacityEffectInstance::Prepare(FReloadResultContext& Context)
{
	const UReloadCapacityEffectDefinition* Config = CastChecked<UReloadCapacityEffectDefinition>(Definition);
	const float Chance = Weapon->ReloadChanceOverride < 0.f ? Config->Chance : Weapon->ReloadChanceOverride;
	if (FMath::FRand() < Chance)
	{
		Context.Capacity = FMath::Max(Context.Capacity, Context.BaseCapacity * Config->CapacityMultiplier);
	}
}

void UReloadCapacityEffectInstance::Revoke()
{
	if (Weapon)
	{
		Weapon->OnPrepareReload.Remove(PrepareHandle);
	}
	Super::Revoke();
}

bool UAttackPatternEffectInstance::Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source)
{
	Weapon = Target->GetOwner()->FindComponentByClass<UWeaponActionComponent>();
	const UAttackPatternEffectDefinition* Pattern = Cast<UAttackPatternEffectDefinition>(Config);
	if (!Weapon || !Pattern || !Super::Grant(Target, Config, Source))
	{
		return false;
	}
	Weapon->SetPattern(GrantSource, Pattern->RoundsPerAttack);
	return true;
}

void UAttackPatternEffectInstance::Revoke()
{
	if (Weapon)
	{
		Weapon->RemovePattern(GrantSource);
	}
	Super::Revoke();
}
