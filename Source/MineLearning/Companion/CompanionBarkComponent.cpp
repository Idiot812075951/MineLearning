#include "CompanionBarkComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UCompanionBarkComponent::UCompanionBarkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FClassFinder<UUserWidget> BarkWidgetFinder(TEXT("/Game/UI/Companion/WBP_CompanionBark"));
	WidgetClass = BarkWidgetFinder.Class;
}

void UCompanionBarkComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (!Owner || !WidgetClass)
	{
		return;
	}

	BarkWidgetComponent = NewObject<UWidgetComponent>(Owner, TEXT("BarkWorldWidget"));
	BarkWidgetComponent->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	BarkWidgetComponent->SetRelativeLocation(RelativeLocation);
	// Screen space still follows the owner in world space, but remains camera-facing
	// and pixel-sized. This avoids a 360 cm billboard drifting across the scene.
	BarkWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	BarkWidgetComponent->SetDrawSize(FVector2D(300.0f, 68.0f));
	BarkWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	BarkWidgetComponent->SetWidgetClass(WidgetClass);
	BarkWidgetComponent->RegisterComponent();
	BarkWidgetComponent->InitWidget();
	BarkWidgetComponent->SetVisibility(false);
}

bool UCompanionBarkComponent::TrySpeak(const FText& Text, FLinearColor Color, bool bShowIcon)
{
	(void)bShowIcon; // Compatibility only: Bark is text-only; result icons are world feedback.
	UWorld* World = GetWorld();
	if (!World || !BarkWidgetComponent || World->GetTimeSeconds() < NextSpeakTime)
	{
		return false;
	}

	BarkWidgetComponent->InitWidget();
	if (UUserWidget* Widget = BarkWidgetComponent->GetUserWidgetObject())
	{
		if (UTextBlock* BarkText = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("BarkText"))))
		{
			BarkText->SetText(Text);
			BarkText->SetColorAndOpacity(FSlateColor(Color));
			BarkText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	BarkWidgetComponent->SetVisibility(true);
	NextSpeakTime = World->GetTimeSeconds() + SpeakCooldown;
	World->GetTimerManager().SetTimer(HideTimerHandle, this, &UCompanionBarkComponent::HideBark, SpeakDuration, false);
	return true;
}

void UCompanionBarkComponent::HideBark()
{
	if (BarkWidgetComponent)
	{
		BarkWidgetComponent->SetVisibility(false);
	}
}
