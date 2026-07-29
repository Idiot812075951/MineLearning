#include "OreHealthBarWidget.h"

#include "MineableOre.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

void UOreHealthBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
	RootSizeBox->SetWidthOverride(340.0f);
	RootSizeBox->SetHeightOverride(52.0f);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HealthOverlay"));
	RootSizeBox->AddChild(Overlay);

	HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthProgressBar"));
	HealthProgressBar->SetPercent(1.0f);
	HealthProgressBar->SetFillColorAndOpacity(FLinearColor(0.85f, 0.05f, 0.03f, 1.0f));
	Overlay->AddChildToOverlay(HealthProgressBar);

	HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
	HealthText->SetJustification(ETextJustify::Center);
	HealthText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	HealthText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	HealthText->SetShadowColorAndOpacity(FLinearColor::Black);

	FSlateFontInfo Font = HealthText->GetFont();
	Font.Size = 22;
	HealthText->SetFont(Font);

	if (UOverlaySlot* TextSlot = Overlay->AddChildToOverlay(HealthText))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	WidgetTree->RootWidget = RootSizeBox;
}

void UOreHealthBarWidget::NativeDestruct()
{
	SetObservedOre(nullptr);
	Super::NativeDestruct();
}

void UOreHealthBarWidget::SetObservedOre(AMineableOre* InOre)
{
	if (ObservedOre == InOre)
	{
		RefreshFromOre();
		return;
	}

	if (IsValid(ObservedOre))
	{
		ObservedOre->OnOreHealthChanged.RemoveDynamic(this, &UOreHealthBarWidget::HandleOreHealthChanged);
	}

	ObservedOre = InOre;

	if (IsValid(ObservedOre))
	{
		ObservedOre->OnOreHealthChanged.RemoveDynamic(this, &UOreHealthBarWidget::HandleOreHealthChanged);
		ObservedOre->OnOreHealthChanged.AddDynamic(this, &UOreHealthBarWidget::HandleOreHealthChanged);
	}

	RefreshFromOre();
}

void UOreHealthBarWidget::HandleOreHealthChanged(float CurrentHealth, float MaxHealth)
{
	const float SafeMaxHealth = FMath::Max(MaxHealth, 0.01f);

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(FMath::Clamp(CurrentHealth / SafeMaxHealth, 0.0f, 1.0f));
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, MaxHealth)));
	}
}

void UOreHealthBarWidget::RefreshFromOre()
{
	if (IsValid(ObservedOre))
	{
		HandleOreHealthChanged(ObservedOre->GetCurrentHealth(), ObservedOre->GetMaxHealth());
		return;
	}

	HandleOreHealthChanged(0.0f, 1.0f);
}
