#include "GrabTargetIndicatorComponent.h"

#include "MineLearning/Interaction/GrabbableComponent.h"
#include "MineLearning/Manifestation/Guren/GurenQSkillComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UGrabTargetIndicatorComponent::UGrabTargetIndicatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGrabTargetIndicatorComponent::BeginPlay()
{
	Super::BeginPlay();
	Source = GetOwner()->FindComponentByClass<UGurenQSkillComponent>();
	if (Source)
	{
		Source->OnTargetsChanged.AddDynamic(this, &UGrabTargetIndicatorComponent::ReconcileIndicators);
		ReconcileIndicators();
	}
}

void UGrabTargetIndicatorComponent::ReconcileIndicators()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const TArray<UGrabbableComponent*> Candidates = Source && Player && Player->IsLocalController()
		? Source->GetCandidates() : TArray<UGrabbableComponent*>();
	for (auto It = Indicators.CreateIterator(); It; ++It)
	{
		if (!Candidates.Contains(It.Key()))
		{
			It.Value()->SetWidget(nullptr);
			It.Value()->DestroyComponent();
			It.RemoveCurrent();
		}
	}
	if (!IndicatorClass || !Player)
	{
		return;
	}
	for (UGrabbableComponent* Candidate : Candidates)
	{
		if (Indicators.Contains(Candidate))
		{
			continue;
		}
		UGrabTargetWidget* Widget = CreateWidget<UGrabTargetWidget>(Player, IndicatorClass);
		if (!Widget)
		{
			continue;
		}
		Widget->Source = Source;
		Widget->Target = Candidate;
		UWidgetComponent* Host = NewObject<UWidgetComponent>(GetOwner());
		Host->SetWidgetSpace(EWidgetSpace::Screen);
		Host->SetOwnerPlayer(Player->GetLocalPlayer());
		Host->SetDrawSize(DrawSize);
		Host->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Host->SetGenerateOverlapEvents(false);
		Host->SetupAttachment(Candidate->GetOwner()->GetRootComponent());
		Host->SetWorldLocation(Candidate->GetIndicatorLocation());
		Host->SetWidget(Widget);
		Host->RegisterComponent();
		Indicators.Add(Candidate, Host);
	}
}

void UGrabTargetIndicatorComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Source)
	{
		Source->OnTargetsChanged.RemoveDynamic(this, &UGrabTargetIndicatorComponent::ReconcileIndicators);
	}
	for (const TPair<TObjectPtr<UGrabbableComponent>, TObjectPtr<UWidgetComponent>>& Entry : Indicators)
	{
		Entry.Value->SetWidget(nullptr);
		Entry.Value->DestroyComponent();
	}
	Indicators.Reset();
	Super::EndPlay(Reason);
}
