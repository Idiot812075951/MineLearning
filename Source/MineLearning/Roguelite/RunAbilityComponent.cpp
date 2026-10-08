#include "RunAbilityComponent.h"
#include "MineLearning/AI/AutonomousUnit.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace RunAbilitySources
{
	const FName Overclock(TEXT("RunAbility:Overclock"));
	const FName Work(TEXT("RunAbility:Work"));
}

bool UOverclockDefinition::IsValidConfiguration() const
{
	return Boost.IsValid() && Overheat.IsValid() && Boost.Duration > 0.f && Overheat.Duration > 0.f
		&& FMath::IsFinite(Cooldown) && Cooldown >= Boost.Duration + Overheat.Duration;
}

bool UAIWorkDefinition::IsValidConfiguration() const
{
	return DisplayRule.IsValid() && FMath::IsFinite(BasePower) && FMath::IsFinite(FocusPower)
		&& BasePower >= 0.f && FocusPower >= 0.f && BasePower + FocusPower <= 100.f;
}

URunAbilityComponent::URunAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URunAbilityComponent::Configure(UOverclockDefinition* InOverclock, UAIWorkDefinition* InWork)
{
	if (!GetOwner()->HasAuthority()) { return; }
	if ((InOverclock && !InOverclock->IsValidConfiguration()) || (InWork && !InWork->IsValidConfiguration())) { return; }
	if (Overclock != InOverclock)
	{
		Overclock = InOverclock;
		ActivationTime = -1.;
		GetWorld()->GetTimerManager().ClearTimer(AbilityTimer);
		RefreshOverclock();
	}
	Work = InWork;
	RefreshAI();
}

void URunAbilityComponent::RegisterUnit(APawn* Unit)
{
	if (!IsValid(Unit)) { return; }
	Units.AddUnique(Unit);
	Unit->OnDestroyed.AddUniqueDynamic(this, &URunAbilityComponent::UnitDestroyed);
	Unit->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &URunAbilityComponent::ControllerChanged);
	if (UHealthComponent* Health = Unit->FindComponentByClass<UHealthComponent>()) { Health->OnHealthChanged.AddUniqueDynamic(this, &URunAbilityComponent::RefreshAI); }
	RefreshAI();
}

void URunAbilityComponent::SetControlledUnit(APawn* Unit)
{
	if (Controlled != Unit)
	{
		if (APawn* Previous = Controlled.Get())
		{
			if (UUnitEffectComponent* Effects = Previous->FindComponentByClass<UUnitEffectComponent>())
			{
				Effects->RemoveEffect(RunAbilitySources::Overclock);
			}
		}
		Controlled = Unit;
	}
	RefreshOverclock();
	RefreshAI();
}

bool URunAbilityComponent::ActivateOverclock()
{
	if (!GetOwner()->HasAuthority() || !Overclock || !Controlled.IsValid() || GetCooldownRemaining() > 0.f
		|| !Overclock->IsValidConfiguration()) { return false; }
	ActivationTime = GetWorld()->GetTimeSeconds();
	PublishClock();
	return true;
}

float URunAbilityComponent::GetCooldownRemaining() const
{
	return Overclock && ActivationTime >= 0. ? FMath::Max(0.f, Overclock->Cooldown - float(GetWorld()->GetTimeSeconds() - ActivationTime)) : 0.f;
}

EOverclockPhase URunAbilityComponent::GetPhase() const
{
	if (!Overclock || ActivationTime < 0.) { return EOverclockPhase::Ready; }
	const double Elapsed = GetWorld()->GetTimeSeconds() - ActivationTime;
	if (Elapsed < Overclock->Boost.Duration) { return EOverclockPhase::Boost; }
	if (Elapsed < Overclock->Boost.Duration + Overclock->Overheat.Duration) { return EOverclockPhase::Overheat; }
	return GetCooldownRemaining() > 0.f ? EOverclockPhase::Cooldown : EOverclockPhase::Ready;
}

void URunAbilityComponent::RefreshOverclock()
{
	UUnitEffectComponent* Effects = Controlled.IsValid() ? Controlled->FindComponentByClass<UUnitEffectComponent>() : nullptr;
	if (!Effects) { return; }
	const EOverclockPhase Phase = GetPhase();
	if (Phase != EOverclockPhase::Boost && Phase != EOverclockPhase::Overheat)
	{
		Effects->RemoveEffect(RunAbilitySources::Overclock);
		return;
	}
	FUnitEffectRule Rule = Phase == EOverclockPhase::Boost ? Overclock->Boost : Overclock->Overheat;
	Rule.Id = RunAbilitySources::Overclock;
	Rule.Duration = float(ActivationTime + Overclock->Boost.Duration
		+ (Phase == EOverclockPhase::Overheat ? Overclock->Overheat.Duration : 0.f) - GetWorld()->GetTimeSeconds());
	Effects->ApplyEffect(Rule);
}

void URunAbilityComponent::PublishClock()
{
	RefreshOverclock();
	OnAbilityChanged.Broadcast();
	const float Remaining = GetCooldownRemaining();
	if (Remaining <= 0.f) { return; }
	const double Elapsed = GetWorld()->GetTimeSeconds() - ActivationTime;
	float Delay = FMath::Min(1.f, Remaining);
	for (float Boundary : {Overclock->Boost.Duration, Overclock->Boost.Duration + Overclock->Overheat.Duration})
	{
		if (Boundary > Elapsed) { Delay = FMath::Min(Delay, float(Boundary - Elapsed)); }
	}
	GetWorld()->GetTimerManager().SetTimer(AbilityTimer, this, &URunAbilityComponent::PublishClock, FMath::Max(0.001f, Delay), false);
}

bool URunAbilityComponent::IsEligibleAI(APawn* Unit) const
{
	const IAutonomousUnit* Capability = Cast<IAutonomousUnit>(Unit);
	const UHealthComponent* Health = IsValid(Unit) ? Unit->FindComponentByClass<UHealthComponent>() : nullptr;
	return IsValid(Unit) && !Unit->IsActorBeingDestroyed() && Unit != Controlled && !Unit->IsPlayerControlled()
		&& Unit->GetController() && Capability && Capability->SupportsAutonomousControl()
		&& Capability->GetAutonomousOutput() != EAutonomousOutput::None && Health
		&& Health->Faction == ECombatFaction::Player && !Health->IsDead();
}

void URunAbilityComponent::RefreshAI()
{
	Units.RemoveAll([](const TWeakObjectPtr<APawn>& Unit) { return !Unit.IsValid(); });
	if (!Work) { Focus.Reset(); }
	else if (!IsEligibleAI(Focus.Get()))
	{
		Focus.Reset();
		float BestDistance = TNumericLimits<float>::Max();
		const FVector Origin = Controlled.IsValid() ? Controlled->GetActorLocation() : GetOwner()->GetActorLocation();
		for (const TWeakObjectPtr<APawn>& Unit : Units)
		{
			if (!IsEligibleAI(Unit.Get())) { continue; }
			const float Distance = FVector::DistSquared(Origin, Unit->GetActorLocation());
			if (Distance < BestDistance) { BestDistance = Distance; Focus = Unit; }
		}
	}
	for (const TWeakObjectPtr<APawn>& Unit : Units)
	{
		UUnitEffectComponent* Effects = Unit->FindComponentByClass<UUnitEffectComponent>();
		if (!Effects) { continue; }
		if (!Work || !IsEligibleAI(Unit.Get())) { Effects->RemoveEffect(RunAbilitySources::Work); continue; }
		const float Power = Work->BasePower + (Unit == Focus ? Work->FocusPower : 0.f);
		FUnitEffectRule Rule = Work->DisplayRule;
		Rule.Id = RunAbilitySources::Work;
		Rule.Duration = 0.f;
		Rule.Modifiers = FCombatModifiers();
		Rule.CarryCapacityPercent = 0.f;
		if (Cast<IAutonomousUnit>(Unit.Get())->GetAutonomousOutput() == EAutonomousOutput::CarryCapacity) { Rule.CarryCapacityPercent = Power; }
		else { Rule.Modifiers.PrimaryDamage = Power; }
		Rule.Status = Unit == Focus ? NSLOCTEXT("Work", "Focus", "主力 AI") : NSLOCTEXT("Work", "Team", "团队强化");
		Rule.GroundRingIntensity = Unit == Focus ? 1.f : 0.3f;
		Rule.bOverheadMarker = Unit == Focus;
		Rule.AuraColor = Unit == Focus ? FLinearColor(1.f, 0.75f, 0.2f) : FLinearColor(0.05f, 0.8f, 1.f);
		Effects->ApplyEffect(Rule, Unit == Focus ? 2 : 1);
	}
	OnAbilityChanged.Broadcast();
}

bool URunAbilityComponent::CycleFocus()
{
	if (!Work || !GetOwner()->HasAuthority()) { return false; }
	TArray<TWeakObjectPtr<APawn>> Eligible;
	for (const TWeakObjectPtr<APawn>& Unit : Units) { if (IsEligibleAI(Unit.Get())) { Eligible.Add(Unit); } }
	if (Eligible.IsEmpty()) { RefreshAI(); return false; }
	const int32 Current = Eligible.IndexOfByKey(Focus);
	Focus = Eligible[(Current + 1) % Eligible.Num()];
	RefreshAI();
	return true;
}

void URunAbilityComponent::UnitDestroyed(AActor* Unit)
{
	Units.RemoveAll([Unit](const TWeakObjectPtr<APawn>& Entry) { return !Entry.IsValid() || Entry.Get() == Unit; });
	if (Focus.Get() == Unit) { Focus.Reset(); }
	RefreshAI();
}

void URunAbilityComponent::ControllerChanged(APawn* Unit, AController* OldController, AController* NewController)
{
	RefreshAI();
}

FText URunAbilityComponent::GetAbilityStatus() const
{
	if (Work)
	{
		if (!Focus.IsValid()) { return NSLOCTEXT("Work", "NoAI", "等待己方 AI"); }
		const UCombatComponent* Combat = Focus->FindComponentByClass<UCombatComponent>();
		const UCombatConfig* Config = Combat ? Combat->GetConfig() : nullptr;
		const FText Name = Config ? Config->DisplayName : NSLOCTEXT("Work", "Worker", "机器人");
		return FText::Format(NSLOCTEXT("Work", "FocusName", "[X] 主力：{0}"), Name);
	}
	if (!Overclock) { return FText::GetEmpty(); }
	if (GetPhase() == EOverclockPhase::Boost) { return NSLOCTEXT("Overclock", "Active", "超频中"); }
	if (GetPhase() == EOverclockPhase::Overheat) { return NSLOCTEXT("Overclock", "Hot", "过热"); }
	return GetCooldownRemaining() > 0.f ? FText::Format(NSLOCTEXT("Overclock", "Cooldown", "超频冷却 {0} 秒"), FText::AsNumber(FMath::CeilToInt(GetCooldownRemaining()))) : NSLOCTEXT("Overclock", "Ready", "[T] 超频就绪");
}

void URunAbilityComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Configure(nullptr, nullptr);
	GetWorld()->GetTimerManager().ClearTimer(AbilityTimer);
	for (const TWeakObjectPtr<APawn>& Unit : Units)
	{
		if (Unit.IsValid())
		{
			Unit->OnDestroyed.RemoveDynamic(this, &URunAbilityComponent::UnitDestroyed);
			Unit->ReceiveControllerChangedDelegate.RemoveDynamic(this, &URunAbilityComponent::ControllerChanged);
			if (UHealthComponent* Health = Unit->FindComponentByClass<UHealthComponent>()) { Health->OnHealthChanged.RemoveDynamic(this, &URunAbilityComponent::RefreshAI); }
		}
	}
	Super::EndPlay(Reason);
}
