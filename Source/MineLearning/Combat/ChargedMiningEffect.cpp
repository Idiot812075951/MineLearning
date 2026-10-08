#include "ChargedMiningEffect.h"
#include "MineLearning/Mining/MiningToolComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UChargedMiningEffectDefinition::UChargedMiningEffectDefinition()
{
	InstanceClass = UChargedMiningEffectInstance::StaticClass();
}

bool UChargedMiningEffectDefinition::SupportsTarget(const AActor* Target) const
{
	return Super::SupportsTarget(Target) && Target->FindComponentByClass<UMiningToolComponent>();
}

bool UChargedMiningEffectDefinition::ValidateDefinition(FString& Error) const
{
	return Super::ValidateDefinition(Error) && InstanceClass->IsChildOf(UChargedMiningEffectInstance::StaticClass())
		&& FMath::IsFinite(ChargeSeconds) && ChargeSeconds > 0.f
		&& FMath::IsFinite(HitReduction) && HitReduction >= 0.f
		&& FMath::IsFinite(DamageMultiplier) && DamageMultiplier >= 0.f
		&& FMath::IsFinite(ImpactScale) && ImpactScale >= 0.f;
}

bool UChargedMiningEffectInstance::Grant(UUnitEffectComponent* Target, UUnitEffectDefinition* Config, FName Source)
{
	Mining = Target->GetOwner()->FindComponentByClass<UMiningToolComponent>();
	if (!Mining || !Cast<UChargedMiningEffectDefinition>(Config) || !Super::Grant(Target, Config, Source)) { return false; }
	StartedHandle = Mining->OnCycleStarted.AddUObject(this, &UChargedMiningEffectInstance::CycleStarted);
	FinishedHandle = Mining->OnCycleFinished.AddUObject(this, &UChargedMiningEffectInstance::CycleFinished);
	PrepareHandle = Mining->OnPrepareHit.AddUObject(this, &UChargedMiningEffectInstance::PrepareHit);
	CommittedHandle = Mining->OnHitCommitted.AddUObject(this, &UChargedMiningEffectInstance::HitCommitted);
	StartCharge();
	return true;
}

void UChargedMiningEffectInstance::StartCharge()
{
	bCharged = false;
	const UChargedMiningEffectDefinition* Config = CastChecked<UChargedMiningEffectDefinition>(Definition);
	Effects->GetWorld()->GetTimerManager().SetTimer(ChargeTimer, this, &UChargedMiningEffectInstance::Charged, Config->ChargeSeconds, false);
	Publish();
}

void UChargedMiningEffectInstance::Charged()
{
	bCharged = true;
	Effects->GetWorld()->GetTimerManager().ClearTimer(ChargeTimer);
	Publish();
}

void UChargedMiningEffectInstance::CycleStarted()
{
	bCycleEligible = bCharged;
	bCycleEmpowered = false;
}

void UChargedMiningEffectInstance::PrepareHit(FMiningHitContext& Context)
{
	if (bCycleEligible || bCycleEmpowered)
	{
		const UChargedMiningEffectDefinition* Config = CastChecked<UChargedMiningEffectDefinition>(Definition);
		Context.DamageMultiplier *= Config->DamageMultiplier;
		Context.ImpactScale *= Config->ImpactScale;
	}
}

void UChargedMiningEffectInstance::HitCommitted()
{
	if (bCycleEmpowered) { return; }
	if (bCycleEligible)
	{
		bCharged = false;
		bCycleEmpowered = true;
		Publish();
		return;
	}
	if (!bCharged)
	{
		FTimerManager& Timers = Effects->GetWorld()->GetTimerManager();
		const float Remaining = Timers.GetTimerRemaining(ChargeTimer) - CastChecked<UChargedMiningEffectDefinition>(Definition)->HitReduction;
		if (Remaining <= 0.f) { Charged(); }
		else { Timers.SetTimer(ChargeTimer, this, &UChargedMiningEffectInstance::Charged, Remaining, false); }
	}
}

void UChargedMiningEffectInstance::CycleFinished()
{
	const bool bConsumed = bCycleEmpowered;
	bCycleEligible = false;
	bCycleEmpowered = false;
	if (bConsumed) { StartCharge(); }
}

void UChargedMiningEffectInstance::Publish()
{
	FUnitEffectRule State = Definition->Rule;
	State.Id = GrantSource;
	State.Duration = 0.f;
	State.Trigger = EUnitEffectTrigger::Persistent;
	State.AuraIntensity = 0.f;
	State.bToolGlow = bCharged || bCycleEmpowered;
	State.Status = bCycleEmpowered ? NSLOCTEXT("Charge", "Active", "整次钻采强化中")
		: bCharged ? NSLOCTEXT("Charge", "Ready", "充能就绪") : NSLOCTEXT("Charge", "Charging", "充能中");
	Effects->ApplyEffect(State);
}

void UChargedMiningEffectInstance::Revoke()
{
	if (Mining)
	{
		Mining->OnCycleStarted.Remove(StartedHandle);
		Mining->OnCycleFinished.Remove(FinishedHandle);
		Mining->OnPrepareHit.Remove(PrepareHandle);
		Mining->OnHitCommitted.Remove(CommittedHandle);
	}
	if (Effects && Effects->GetWorld()) { Effects->GetWorld()->GetTimerManager().ClearTimer(ChargeTimer); }
	Super::Revoke();
}
