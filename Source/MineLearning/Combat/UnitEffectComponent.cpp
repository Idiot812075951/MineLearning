#include "UnitEffectComponent.h"

#include "CombatComponent.h"
#include "HealthComponent.h"
#include "UnitEffectDefinition.h"
#include "Engine/World.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "UnitEffects"

bool FUnitEffectRule::IsValid() const
{
	return !Id.IsNone() && FMath::IsFinite(AuraIntensity) && AuraIntensity >= 0.f && AuraIntensity <= 1.f && Modifiers.IsValid() && FMath::IsFinite(Chance) && Chance >= 0.f && Chance <= 1.f && FMath::IsFinite(Duration) &&
		(Trigger == EUnitEffectTrigger::Persistent ? Duration >= 0.f : Duration > 0.f && !EventName.IsNone());
}

UUnitEffectComponent::UUnitEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUnitEffectComponent::BeginPlay()
{
	Super::BeginPlay();
	Combat = GetOwner()->FindComponentByClass<UCombatComponent>();
	Health = GetOwner()->FindComponentByClass<UHealthComponent>();
	if (GetOwner()->HasAuthority() && Health)
	{
		Health->OnHealthChanged.AddUniqueDynamic(this, &UUnitEffectComponent::HealthChanged);
	}
}

void UUnitEffectComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Health)
	{
		Health->OnHealthChanged.RemoveDynamic(this, &UUnitEffectComponent::HealthChanged);
	}
	ClearEffects();
	Super::EndPlay(Reason);
}

void UUnitEffectComponent::HandleEvent(FName EventName)
{
	if (EventName.IsNone() || !GetOwner()->HasAuthority() || !Combat || (Health && Health->IsDead()))
	{
		return;
	}
	TArray<TObjectPtr<UUnitEffectInstance>> InstanceSnapshot;
	Instances.GenerateValueArray(InstanceSnapshot);
	for (UUnitEffectInstance* Instance : InstanceSnapshot)
	{
		Instance->HandleEvent(EventName);
	}

}

bool UUnitEffectComponent::ApplyEffect(const FUnitEffectRule& Effect, int32 StackCount)
{
	if (!Combat || !GetOwner()->HasAuthority() || !Effect.IsValid() || (Health && Health->IsDead()))
	{
		return false;
	}
	FActiveUnitEffect& Active = ActiveEffects.FindOrAdd(Effect.Id);
	Active.Definition = Effect;
	Active.StackCount = StackCount;
	Active.EndTime = Effect.Duration > 0.f ? GetWorld()->GetTimeSeconds() + Effect.Duration : 0.f;
	Combat->SetModifier(Effect.Id, Effect.Modifiers);
	ScheduleNextChange();
	OnEffectsChanged.Broadcast();
	return true;
}

void UUnitEffectComponent::RemoveEffect(FName EffectId)
{
	if (!GetOwner()->HasAuthority() || ActiveEffects.Remove(EffectId) == 0)
	{
		return;
	}
	if (Combat)
	{
		Combat->RemoveModifier(EffectId);
	}
	ScheduleNextChange();
	OnEffectsChanged.Broadcast();
}

void UUnitEffectComponent::RemoveAllEffects()
{
	TArray<FName> Ids;
	ActiveEffects.GetKeys(Ids);
	ActiveEffects.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChangeTimer);
	}
	for (FName Id : Ids)
	{
		if (Combat)
		{
			Combat->RemoveModifier(Id);
		}
	}
	OnEffectsChanged.Broadcast();
}

void UUnitEffectComponent::ClearEffects()
{
	// Clear contributions before detaching hooks. During world destruction a
	// component can receive EndPlay after its world/timer manager was released.
	RemoveAllEffects();
	const TArray<FName> Sources = GetGrantedSources();
	for (FName Source : Sources)
	{
		RevokeDefinition(Source);
	}
}

void UUnitEffectComponent::HealthChanged()
{
	if (Health && Health->IsDead())
	{
		ClearEffects();
	}
}

float UUnitEffectComponent::GetRemainingTime(FName RuleId) const
{
	const FActiveUnitEffect* Effect = ActiveEffects.Find(RuleId);
	return Effect && Effect->EndTime > 0.f ? FMath::Max(0.f, Effect->EndTime - GetWorld()->GetTimeSeconds()) : 0.f;
}

void UUnitEffectComponent::ScheduleNextChange()
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	Timers.ClearTimer(ChangeTimer);
	float Delay = TNumericLimits<float>::Max();
	for (const TPair<FName, FActiveUnitEffect>& Pair : ActiveEffects)
	{
		if (Pair.Value.EndTime > 0.f)
		{
			const float Remaining = GetRemainingTime(Pair.Key);
			// Schedule the next displayed-second boundary or expiry; no widget polling.
			const float NextBoundary = Remaining - FMath::Max(0, FMath::CeilToInt(Remaining - 0.001f) - 1);
			Delay = FMath::Min(Delay, FMath::Max(0.001f, NextBoundary));
		}
	}
	if (Delay != TNumericLimits<float>::Max())
	{
		Timers.SetTimer(ChangeTimer, this, &UUnitEffectComponent::TimeBoundaryReached, Delay, false);
	}
}

void UUnitEffectComponent::TimeBoundaryReached()
{
	TArray<FName> Expired;
	for (const TPair<FName, FActiveUnitEffect>& Pair : ActiveEffects)
	{
		if (Pair.Value.EndTime > 0.f && GetRemainingTime(Pair.Key) <= 0.f)
		{
			Expired.Add(Pair.Key);
		}
	}
	for (FName Id : Expired)
	{
		if (const TObjectPtr<UUnitEffectInstance>* Instance = Instances.Find(Id); Instance && (*Instance)->HandleExpiry())
		{
			continue;
		}
		// Timed equipment must also release its action hooks. An event-triggered
		// effect only loses its active contribution and stays equipped.
		if (ActiveEffects[Id].Definition.Trigger == EUnitEffectTrigger::Persistent && Instances.Contains(Id))
		{
			RevokeDefinition(Id);
		}
		else
		{
			RemoveEffect(Id);
		}
	}
	ScheduleNextChange();
	OnEffectsChanged.Broadcast();
}

FText UUnitEffectComponent::GetEffectSummary() const
{
	TArray<FName> Ids;
	ActiveEffects.GetKeys(Ids);
	Ids.Sort(FNameLexicalLess());
	TArray<FText> Lines;
	for (FName Id : Ids)
	{
		const FActiveUnitEffect& Active = ActiveEffects[Id];
		const FText Name = Active.Definition.DisplayName.IsEmpty() ? FText::FromName(Id) : Active.Definition.DisplayName;
		const FText Duration = Active.EndTime > 0.f
			? FText::Format(LOCTEXT("Remaining", "{0} 秒"), FText::AsNumber(FMath::Max(1, FMath::CeilToInt(GetRemainingTime(Id) - 0.001f))))
			: LOCTEXT("Persistent", "常驻");
		Lines.Add(Active.StackCount > 0
			? FText::Format(LOCTEXT("StackedEffectRow", "{0} ×{1}  ·  {2}"), Name, FText::AsNumber(Active.StackCount), Duration)
			: FText::Format(LOCTEXT("EffectRow", "{0}  ·  {1}"), Name, Duration));
	}
	return FText::Join(FText::FromString(TEXT("\n")), Lines);
}

#undef LOCTEXT_NAMESPACE

TArray<FUnitEffectView> UUnitEffectComponent::GetActiveEffectViews() const
{
	TArray<FUnitEffectView> Result;
	for (const TPair<FName, FActiveUnitEffect>& Pair : ActiveEffects)
	{
		FUnitEffectView& View = Result.AddDefaulted_GetRef();
		View.Source = Pair.Key;
		View.Name = Pair.Value.Definition.DisplayName;
		View.Icon = Pair.Value.Definition.Icon;
		View.AuraIntensity = Pair.Value.Definition.AuraIntensity;
		View.bTimed = Pair.Value.EndTime > 0.f;
		View.Remaining = GetRemainingTime(Pair.Key);
		View.StackCount = Pair.Value.StackCount;
		const FText Duration = View.bTimed
			? FText::Format(NSLOCTEXT("UnitEffect", "Remaining", "{0} 秒"), FText::AsNumber(FMath::Max(1, FMath::CeilToInt(View.Remaining - 0.001f))))
			: NSLOCTEXT("UnitEffect", "Persistent", "常驻");
		View.StatusText = View.StackCount > 0
			? FText::Format(NSLOCTEXT("UnitEffect", "Stacked", "{0} ×{1} · {2}"), View.Name, FText::AsNumber(View.StackCount), Duration)
			: FText::Format(NSLOCTEXT("UnitEffect", "Status", "{0} · {1}"), View.Name, Duration);
	}
	Result.Sort([](const FUnitEffectView& A, const FUnitEffectView& B) { return A.Source.LexicalLess(B.Source); });
	return Result;
}

bool UUnitEffectComponent::GrantDefinition(FName Source, UUnitEffectDefinition* Definition)
{
	FString Error;
	if (Source.IsNone() || !Definition || !Definition->ValidateDefinition(Error) || !Definition->SupportsTarget(GetOwner()) || !Combat
		|| !GetOwner()->HasAuthority() || (Health && Health->IsDead()))
	{
		return false;
	}
	if (Instances.Contains(Source))
	{
		return true;
	}
	UUnitEffectInstance* Instance = NewObject<UUnitEffectInstance>(this, Definition->InstanceClass);
	Instances.Add(Source, Instance);
	if (!Instance->Grant(this, Definition, Source))
	{
		Instance->Revoke();
		Instances.Remove(Source);
		return false;
	}
	return true;
}

void UUnitEffectComponent::RevokeDefinition(FName Source)
{
	TObjectPtr<UUnitEffectInstance> Instance;
	if (Instances.RemoveAndCopyValue(Source, Instance))
	{
		Instance->Revoke();
	}
}

TArray<FName> UUnitEffectComponent::GetGrantedSources() const
{
	TArray<FName> Sources;
	Instances.GetKeys(Sources);
	return Sources;
}

void UUnitEffectComponent::RefreshRuntimeState()
{
	for (const TPair<FName, TObjectPtr<UUnitEffectInstance>>& Pair : Instances)
	{
		Pair.Value->RefreshState();
	}
}
