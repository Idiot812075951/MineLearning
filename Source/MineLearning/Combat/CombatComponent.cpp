#include "CombatComponent.h"
#include "CombatDamageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HealthComponent.h"
#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/Manifestation/Guren/GurenQSkillComponent.h"
#include "MineLearning/Interaction/GrabbableComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"

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
	Result.Strength = FMath::Max(0.f, (Result.Strength + AttributeBonus.Strength + EffectiveModifiers.Attributes.Strength) * FMath::Max(0.f, 1.f + EffectiveModifiers.AttributePercent.Strength));
	Result.Agility = FMath::Max(0.f, (Result.Agility + AttributeBonus.Agility + EffectiveModifiers.Attributes.Agility) * FMath::Max(0.f, 1.f + EffectiveModifiers.AttributePercent.Agility));
	Result.Intelligence = FMath::Max(0.f, (Result.Intelligence + AttributeBonus.Intelligence + EffectiveModifiers.Attributes.Intelligence) * FMath::Max(0.f, 1.f + EffectiveModifiers.AttributePercent.Intelligence));
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
	DOREPLIFETIME(UCombatComponent, EffectiveModifiers);
}

bool UCombatComponent::SetModifier(FName Source, const FCombatModifiers& Modifier)
{
	if (!GetOwner()->HasAuthority() || Source.IsNone() || !Modifier.IsValid())
	{
		return false;
	}
	Modifiers.Add(Source, Modifier);
	RecomputeModifiers();
	return true;
}

void UCombatComponent::RemoveModifier(FName Source)
{
	if (GetOwner()->HasAuthority() && Modifiers.Remove(Source) > 0)
	{
		RecomputeModifiers();
	}
}

void UCombatComponent::RecomputeModifiers()
{
	EffectiveModifiers = {};
	// Stable ordering also defines the winner if configured weapon styles conflict.
	TArray<FName> Sources;
	Modifiers.GetKeys(Sources);
	Sources.Sort(FNameLexicalLess());
	for (FName Source : Sources)
	{
		EffectiveModifiers.Add(Modifiers[Source]);
	}
	OnAttributesChanged.Broadcast();
}

float UCombatComponent::EvaluateDamage(FName SkillId) const
{
	const UCombatConfig* Definition = GetConfig();
	const FSkillDamageSpec* Skill = Definition ? Definition->FindSkill(SkillId) : nullptr;
	return Skill && Skill->bDealsDamage ? FMath::Max(0.f, Skill->Evaluate(GetAttributes())
		+ (SkillId == PrimarySkillId ? EffectiveModifiers.PrimaryDamageFlat : 0.f))
		* (SkillId == PrimarySkillId ? FMath::Max(0.f, 1.f + EffectiveModifiers.PrimaryDamage) : 1.f) : 0.f;
}

float UCombatComponent::GetAttackRange() const
{
	const UCombatConfig* Definition = GetConfig();
	return (Definition ? Definition->AttackRange : 200.f) * GetAttackRangeScale();
}

float UCombatComponent::GetAttackInterval(float BaseInterval) const
{
	const UCombatConfig* Definition = GetConfig();
	const float Cap = FMath::Max(0.1f, Definition ? Definition->MaxAttacksPerSecond : 5.f);
	return FMath::Max(1.f / Cap, FMath::Max(0.001f, BaseInterval) / GetAttackSpeedScale());
}

float UCombatComponent::GetAttackPlayRate(float BaseHitInterval) const
{
	return FMath::Max(0.001f, BaseHitInterval) / GetAttackInterval(BaseHitInterval);
}

float UCombatComponent::GetMaxMoveSpeed() const
{
	const UCombatConfig* Definition = GetConfig();
	return FMath::Max(1.f, Definition ? Definition->MaxMoveSpeed : 1000.f);
}

float UCombatComponent::GetAttackDistance(const AActor* Target) const
{
	if (!IsValid(Target)) { return TNumericLimits<float>::Max(); }
	float Distance = FVector::Distance(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	TInlineComponentArray<UPrimitiveComponent*> Components(Target);
	for (const UPrimitiveComponent* Component : Components)
	{
		if (!Component->IsCollisionEnabled()) { continue; }
		FVector ClosestPoint;
		const float SurfaceDistance = Component->GetClosestPointOnCollision(GetOwner()->GetActorLocation(), ClosestPoint);
		if (SurfaceDistance >= 0.f) { Distance = FMath::Min(Distance, SurfaceDistance); }
	}
	return Distance;
}

bool UCombatComponent::IsInAttackRange(const AActor* Target) const
{
	return GetAttackDistance(Target) <= GetAttackRange();
}

AActor* UCombatComponent::FindNearestAttackTarget() const
{
	AActor* Best = nullptr;
	float BestDistance = GetAttackRange();
	const FVector Origin = GetOwner()->GetActorLocation();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (!UCombatDamageSubsystem::CanDamageTarget(GetOwner(), Candidate)) { continue; }
		const float Distance = GetAttackDistance(Candidate);
		if (Distance > BestDistance) { continue; }
		FCollisionQueryParams Query(SCENE_QUERY_STAT(MeleeTargetSight), false, GetOwner());
		Query.AddIgnoredActor(Candidate);
		if (GetWorld()->LineTraceTestByChannel(Origin, Candidate->GetActorLocation(), ECC_Visibility, Query)) { continue; }
		Best = Candidate;
		BestDistance = Distance;
	}
	return Best;
}

void UCombatComponent::NotifyAttackResolved(bool bHit, bool bCheckAimOnMiss)
{
	if (GetOwner()->HasAuthority())
	{
		// Empty melee swings still detect a valid target beyond reach for player feedback.
		const APawn* Pawn = Cast<APawn>(GetOwner());
		if (!bHit && bCheckAimOnMiss && Pawn && Pawn->IsPlayerControlled())
		{
			FVector Origin;
			FRotator View;
			Pawn->GetController()->GetPlayerViewPoint(Origin, View);
			FHitResult Hit;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(AttackMissRange), true, Pawn);
			if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, Origin + View.Vector() * 10000.f, ECC_Visibility, Query)
				&& UCombatDamageSubsystem::CanDamageTarget(Pawn, Hit.GetActor()) && !IsInAttackRange(Hit.GetActor()))
			{
				NotifyAttackOutOfRange();
			}
		}
		OnPrimaryAttackResolved.Broadcast(bHit, GetOwner()->GetVelocity().SizeSquared2D() > FMath::Square(10.f));
	}
}

bool UCombatComponent::IsStrafing() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const float Yaw = Pawn && Pawn->IsPlayerControlled() ? Pawn->GetControlRotation().Yaw : GetOwner()->GetActorRotation().Yaw;
	const FVector LocalVelocity = FRotator(0.f, Yaw, 0.f).UnrotateVector(GetOwner()->GetVelocity());
	return LocalVelocity.SizeSquared2D() > FMath::Square(10.f) && FMath::Abs(LocalVelocity.Y) > FMath::Abs(LocalVelocity.X);
}

void UCombatComponent::NotifyAttackOutOfRange()
{
	if (GetOwner()->HasAuthority()) { OnAttackOutOfRange.Broadcast(); }
}

bool UCombatComponent::RollAIAttackMiss() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->GetController() && !Pawn->IsPlayerControlled()
		&& FMath::FRand() < FMath::Clamp(EffectiveModifiers.AIMissChance, 0.f, 1.f);
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
	Lines.Add(FText::Format(NSLOCTEXT("Combat", "SpeedModifiers", "移速倍率 {0} | 攻速倍率 {1} | 施法倍率 {2}"),
		FText::AsNumber(GetMoveSpeedScale()), FText::AsNumber(GetAttackSpeedScale()), FText::AsNumber(GetCastSpeedScale())));
	Lines.Add(FText::Format(NSLOCTEXT("Combat", "AttackLimits", "普攻距离 {0} 米 · 移速上限 {1} cm/s · 普攻上限 {2} 次/秒"),
		FText::AsNumber(GetAttackRange() / 100.f), FText::AsNumber(GetMaxMoveSpeed()),
		FText::AsNumber(Definition ? Definition->MaxAttacksPerSecond : 5.f)));
	if (Definition)
	{
		for (const FSkillDamageSpec& Spec : Definition->Skills)
		{
			FCombatSkillViewData& Skill = Result.Skills.AddDefaulted_GetRef();
			Skill.Spec = Spec;
			Skill.Contributions.Strength = Result.Attributes.Strength * Spec.StrengthScale;
			Skill.Contributions.Agility = Result.Attributes.Agility * Spec.AgilityScale;
			Skill.Contributions.Intelligence = Result.Attributes.Intelligence * Spec.IntelligenceScale;
			Skill.Damage = EvaluateDamage(Spec.SkillId);
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
