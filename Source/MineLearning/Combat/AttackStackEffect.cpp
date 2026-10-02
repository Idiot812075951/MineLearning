#include "AttackStackEffect.h"
#include "CombatComponent.h"

UAttackStackEffectDefinition::UAttackStackEffectDefinition()
{
	InstanceClass = UAttackStackEffectInstance::StaticClass();
	Rule.Trigger = EUnitEffectTrigger::Event;
	Rule.EventName = TEXT("PrimaryAttackResolved");
	Rule.Duration = 5.f;
}

bool UAttackStackEffectDefinition::ValidateDefinition(FString& Error) const
{
	const bool bValid = Super::ValidateDefinition(Error)
		&& InstanceClass->IsChildOf(UAttackStackEffectInstance::StaticClass())
		&& Rule.Trigger == EUnitEffectTrigger::Event && Rule.Duration > 0.f
		&& StacksPerHit > 0 && StacksPerStrafeHit > 0 && FMath::IsFinite(AuraPerStack) && AuraPerStack >= 0.f
		&& MaxStacks >= 0 && FMath::IsFinite(DecayInterval) && DecayInterval >= 0.01f
		&& FullStackModifiers.IsValid() && EquippedModifiers.IsValid();
	if (!bValid) { Error = TEXT("Invalid attack stack rule: ") + GetName(); }
	return bValid;
}

FText UAttackStackEffectDefinition::GetRuleDescription() const
{
	TArray<FText> Lines;
	if (bRequiresMovement)
	{
		Lines.Add(FText::Format(NSLOCTEXT("AttackStacks", "Movement", "移动且命中：左右 +{0} 层，前后 +{1} 层；斜向按主方向。"),
			FText::AsNumber(StacksPerStrafeHit), FText::AsNumber(StacksPerHit)));
	}
	Lines.Add(FText::Format(NSLOCTEXT("AttackStacks", "Decay", "命中刷新 {0} 秒；超时后每 {1} 秒减少 1 层，直到清空。"),
		FText::AsNumber(Rule.Duration), FText::AsNumber(DecayInterval)));
	if (EquippedModifiers.AIMissChance > 0.f)
	{
		Lines.Add(FText::Format(NSLOCTEXT("AttackStacks", "AIMiss", "AI 持有此效果时，每次攻击有 {0}% 概率失手；玩家不受此概率影响。"),
			FText::AsNumber(EquippedModifiers.AIMissChance * 100.f)));
	}
	if (bClearOnMiss) { Lines.Add(NSLOCTEXT("AttackStacks", "MissClears", "任何未命中（含空放、超出距离、AI 失手）立即清空层数。")); }
	return FText::Join(FText::FromString(TEXT("\n")), Lines);
}

bool UAttackStackEffectInstance::Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source)
{
	const UAttackStackEffectDefinition* StackConfig = Cast<UAttackStackEffectDefinition>(Config);
	Combat = Target->GetOwner()->FindComponentByClass<UCombatComponent>();
	if (!StackConfig || !Combat || !Super::Grant(Target, Config, Source)) { return false; }
	Combat->SetModifier(GetEquippedSource(), StackConfig->EquippedModifiers);
	AttackHandle = Combat->OnPrimaryAttackResolved.AddUObject(this, &UAttackStackEffectInstance::AttackResolved);
	return true;
}

FName UAttackStackEffectInstance::GetEquippedSource() const
{
	return FName(*(GrantSource.ToString() + TEXT(":Equipped")));
}

void UAttackStackEffectInstance::Revoke()
{
	if (Combat)
	{
		Combat->OnPrimaryAttackResolved.Remove(AttackHandle);
		Combat->RemoveModifier(GetEquippedSource());
	}
	Stacks = 0;
	Super::Revoke();
}

void UAttackStackEffectInstance::AttackResolved(bool bHit, bool bMoving)
{
	const UAttackStackEffectDefinition* Config = CastChecked<UAttackStackEffectDefinition>(Definition);
	if (!bHit)
	{
		if (Config->bClearOnMiss)
		{
			Stacks = 0;
			Effects->RemoveEffect(GrantSource);
		}
		return;
	}
	if ((Config->bRequiresMovement && !bMoving) || FMath::FRand() >= Config->Rule.Chance) { return; }
	const int32 Added = Combat->IsStrafing() ? Config->StacksPerStrafeHit : Config->StacksPerHit;
	const int64 Limit = Config->MaxStacks > 0 ? Config->MaxStacks : MAX_int32;
	Stacks = static_cast<int32>(FMath::Min(Limit, static_cast<int64>(Stacks) + Added));
	ApplyStacks(Config->Rule.Duration);
}

void UAttackStackEffectInstance::ApplyStacks(float Duration)
{
	const UAttackStackEffectDefinition* Config = CastChecked<UAttackStackEffectDefinition>(Definition);
	FUnitEffectRule Applied = Config->Rule;
	Applied.Id = GrantSource;
	Applied.Duration = Duration;
	Applied.AuraIntensity = FMath::Clamp(Config->AuraPerStack * Stacks, 0.f, 1.f);
	Applied.Modifiers = {};
	Applied.Modifiers.Add(Config->Rule.Modifiers, static_cast<float>(Stacks));
	if (Config->MaxStacks > 0 && Stacks == Config->MaxStacks) { Applied.Modifiers.Add(Config->FullStackModifiers); }
	Effects->ApplyEffect(Applied, Stacks);
}

bool UAttackStackEffectInstance::HandleExpiry()
{
	Stacks = FMath::Max(0, Stacks - 1);
	if (Stacks > 0)
	{
		ApplyStacks(CastChecked<UAttackStackEffectDefinition>(Definition)->DecayInterval);
	}
	else
	{
		Effects->RemoveEffect(GrantSource);
	}
	return true;
}
