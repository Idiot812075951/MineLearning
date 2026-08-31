#include "CompanionBarkComponent.h"

#include "TimerManager.h"

UCompanionBarkComponent::UCompanionBarkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCompanionBarkComponent::TrySpeak(
	const FText& Text,
	FLinearColor Color,
	bool bShowIcon,
	bool bIgnoreCooldown)
{
	UWorld* World = GetWorld();
	if (!World || (!bIgnoreCooldown && World->GetTimeSeconds() < NextSpeakTime))
	{
		return false;
	}

	OnBarkRequested.Broadcast(Text, Color, bShowIcon);
	OnBarkVisibilityChanged.Broadcast(true);
	NextSpeakTime = World->GetTimeSeconds() + SpeakCooldown;
	World->GetTimerManager().SetTimer(HideTimerHandle, this, &UCompanionBarkComponent::HideBark, SpeakDuration, false);
	return true;
}

void UCompanionBarkComponent::HideBark()
{
	OnBarkVisibilityChanged.Broadcast(false);
}
