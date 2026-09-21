#include "CombatComponent.h"
#include "HealthComponent.h"
#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/Manifestation/Guren/GurenQSkillComponent.h"
#include "MineLearning/Interaction/GrabbableComponent.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	const UCombatConfig* Definition = GetConfig();
	if (UHealthComponent* Health = GetOwner()->FindComponentByClass<UHealthComponent>(); Health && Definition)
	{
		Health->InitializeHealth(Definition->MaxHealth);
	}
}

FCombatAttributes UCombatComponent::GetAttributes() const
{
	const UCombatConfig* Definition = GetConfig();
	FCombatAttributes Result = Definition ? Definition->Attributes : FCombatAttributes();
	Result.Strength = FMath::Max(0.f, Result.Strength + AttributeBonus.Strength);
	Result.Agility = FMath::Max(0.f, Result.Agility + AttributeBonus.Agility);
	Result.Intelligence = FMath::Max(0.f, Result.Intelligence + AttributeBonus.Intelligence);
	return Result;
}

void UCombatComponent::SetAttributeBonus(FCombatAttributes Bonus)
{
	if (!GetOwner()->HasAuthority() || !FMath::IsFinite(Bonus.Strength) || !FMath::IsFinite(Bonus.Agility) || !FMath::IsFinite(Bonus.Intelligence))
	{
		return;
	}
	AttributeBonus = Bonus;
	OnAttributesChanged.Broadcast();
}

void UCombatComponent::OnRep_Bonus() { OnAttributesChanged.Broadcast(); }

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, AttributeBonus);
}

FCombatPanelViewData UCombatComponent::GetPanelData() const
{
	FCombatPanelViewData Result;
	const UCombatConfig* Definition = GetConfig();
	Result.Name = Definition ? Definition->DisplayName : FText::FromString(GetOwner()->GetName());
	Result.Attributes = GetAttributes();
	if (const UHealthComponent* Health = GetOwner()->FindComponentByClass<UHealthComponent>())
	{
		Result.Health = Health->GetHealth();
		Result.MaxHealth = Health->GetMaxHealth();
	}
	TArray<FText> Lines;
	Lines.Add(FText::Format(NSLOCTEXT("Combat", "PanelHeader", "{0}\n生命  {1} / {2}\n\n力量 Strength  {3}    敏捷 Agility  {4}    能量 Energy  {5}\n\n技能与计算"),
		Result.Name, FText::AsNumber(Result.Health), FText::AsNumber(Result.MaxHealth),
		FText::AsNumber(Result.Attributes.Strength), FText::AsNumber(Result.Attributes.Agility), FText::AsNumber(Result.Attributes.Intelligence)));
	if (Definition)
	{
		for (const FSkillDamageSpec& Spec : Definition->Skills)
		{
			FCombatSkillViewData& Skill = Result.Skills.AddDefaulted_GetRef();
			Skill.Spec = Spec;
			Skill.Contributions.Strength = Result.Attributes.Strength * Spec.StrengthScale;
			Skill.Contributions.Agility = Result.Attributes.Agility * Spec.AgilityScale;
			Skill.Contributions.Intelligence = Result.Attributes.Intelligence * Spec.IntelligenceScale;
			Skill.Damage = Spec.Evaluate(Result.Attributes);
			const int32 FirstLine = Lines.Num();
			Lines.Add(FText::Format(NSLOCTEXT("Combat", "SkillTitle", "\n{0} · {1}\n{2}"), Spec.Input, Spec.Name, Spec.Description));
			if (Spec.bDealsDamage)
			{
				Lines.Add(FText::Format(NSLOCTEXT("Combat", "Formula", "固定 {0} + 力量 {1}×{2} + 敏捷 {3}×{4} + 能量 {5}×{6}\n属性贡献 {7} + {8} + {9} → 普通伤害 {10}"),
					FText::AsNumber(Spec.BaseDamage), FText::AsNumber(Result.Attributes.Strength), FText::AsNumber(Spec.StrengthScale),
					FText::AsNumber(Result.Attributes.Agility), FText::AsNumber(Spec.AgilityScale), FText::AsNumber(Result.Attributes.Intelligence), FText::AsNumber(Spec.IntelligenceScale),
					FText::AsNumber(Skill.Contributions.Strength), FText::AsNumber(Skill.Contributions.Agility), FText::AsNumber(Skill.Contributions.Intelligence), FText::AsNumber(Skill.Damage)));
			}
			TArray<FText> SkillLines;
			for (int32 Index = FirstLine; Index < Lines.Num(); ++Index)
			{
				SkillLines.Add(Lines[Index]);
			}
			Skill.Description = FText::Join(FText::FromString(TEXT("\n")), SkillLines);
		}
	}
	if (const AGunnerCharacter* Gunner = Cast<AGunnerCharacter>(GetOwner()))
	{
		Lines.Add(Gunner->GetCombatMechanics());
	}
	if (const UGurenQSkillComponent* Q = GetOwner()->FindComponentByClass<UGurenQSkillComponent>())
	{
		Lines.Add(FText::Format(NSLOCTEXT("Combat", "ExecuteRules", "\n辐射临界\n固定线 ≤ {0} HP  或  最大生命的 {1}%（满足任一项）\nQ：仅临界目标有图标；抓取成功后必定处决，回血不取消。\n降临：用命中前生命判断；未达临界只受普通伤害。"),
			FText::AsNumber(Q->ExecuteHealthFlat), FText::AsNumber(Q->ExecuteHealthPercent * 100.f)));
		const AActor* Target = Q->GetTarget() ? Q->GetTarget() : (Q->GetSelectedTarget() ? Q->GetSelectedTarget()->GetOwner() : nullptr);
		if (const UHealthComponent* TargetHealth = Target ? Target->FindComponentByClass<UHealthComponent>() : nullptr)
		{
			Lines.Add(FText::Format(NSLOCTEXT("Combat", "ExecuteTarget", "当前目标 {0} / {1} HP · 崩解线 {2} HP"), FText::AsNumber(TargetHealth->GetHealth()), FText::AsNumber(TargetHealth->GetMaxHealth()), FText::AsNumber(Q->GetExecuteThreshold(TargetHealth->GetMaxHealth()))));
		}
	}
	Result.Details = FText::Join(FText::FromString(TEXT("\n")), Lines);
	return Result;
}
