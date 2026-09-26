#include "GunnerHitFeedbackComponent.h"

#include "MineLearning/AI/GunnerCharacter.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UGunnerHitFeedbackComponent::UGunnerHitFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGunnerHitFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();
	Source = Cast<AGunnerCharacter>(GetOwner());
	if (Source)
	{
		Source->OnCriticalHit.AddUniqueDynamic(this, &UGunnerHitFeedbackComponent::HandleCriticalHit);
	}
}

void UGunnerHitFeedbackComponent::HandleCriticalHit(bool bGolden, AActor* Target, FVector TargetTop)
{
	ClearFeedback();
	APlayerController* Player = GetWorld()->GetFirstPlayerController();
	if (!FeedbackClass || !Player || !Player->IsLocalController())
	{
		return;
	}
	UGunnerHitFeedbackWidget* Widget = CreateWidget<UGunnerHitFeedbackWidget>(Player, FeedbackClass);
	if (!Widget)
	{
		return;
	}
	Widget->bGoldenHit = bGolden;
	ActiveFeedback = NewObject<UWidgetComponent>(GetOwner());
	ActiveFeedback->SetWidgetSpace(EWidgetSpace::Screen);
	ActiveFeedback->SetOwnerPlayer(Player->GetLocalPlayer());
	ActiveFeedback->SetDrawSize(DrawSize);
	ActiveFeedback->SetPivot(FVector2D(0.5f, 1.f));
	ActiveFeedback->SetWindowFocusable(false);
	ActiveFeedback->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ActiveFeedback->SetGenerateOverlapEvents(false);
	if (IsValid(Target))
	{
		ActiveFeedback->SetupAttachment(Target->GetRootComponent());
	}
	ActiveFeedback->SetWorldLocation(TargetTop + FVector(0.f, 0.f, HeightOffset));
	ActiveFeedback->SetWidget(Widget);
	ActiveFeedback->RegisterComponent();
	GetWorld()->GetTimerManager().SetTimer(ExpiryHandle, this, &UGunnerHitFeedbackComponent::ClearFeedback, Duration, false);
}

void UGunnerHitFeedbackComponent::ClearFeedback()
{
	GetWorld()->GetTimerManager().ClearTimer(ExpiryHandle);
	if (IsValid(ActiveFeedback))
	{
		ActiveFeedback->SetWidget(nullptr);
		ActiveFeedback->DestroyComponent();
	}
	ActiveFeedback = nullptr;
}

void UGunnerHitFeedbackComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Source)
	{
		Source->OnCriticalHit.RemoveDynamic(this, &UGunnerHitFeedbackComponent::HandleCriticalHit);
	}
	ClearFeedback();
	Super::EndPlay(Reason);
}
