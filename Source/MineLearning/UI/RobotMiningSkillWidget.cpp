#include "RobotMiningSkillWidget.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"

void URobotMiningSkillWidget::SetMiningAvailable(bool bAvailable)
{
	if (bMiningAvailable == bAvailable)
	{
		return;
	}

	bMiningAvailable = bAvailable;
	InvalidateLayoutAndVolatility();
}

void URobotMiningSkillWidget::SetPickupState(bool bAvailable, bool bInCarryFull)
{
	if (bPickupAvailable == bAvailable && bCarryFull == bInCarryFull)
	{
		return;
	}

	bPickupAvailable = bAvailable;
	bCarryFull = bInCarryFull;
	InvalidateLayoutAndVolatility();
}

TSharedRef<SWidget> URobotMiningSkillWidget::RebuildWidget()
{
	return SNew(SBox)
		.WidthOverride(204.0f)
		.HeightOverride(96.0f);
}

int32 URobotMiningSkillWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FLinearColor ActiveColor(1.0f, 0.62f, 0.08f, 1.0f);
	const FLinearColor PickupActiveColor(0.12f, 0.78f, 0.52f, 1.0f);
	const FLinearColor DisabledColor(0.28f, 0.28f, 0.28f, 1.0f);
	const FLinearColor FullColor(0.62f, 0.14f, 0.12f, 1.0f);
	const FLinearColor MiningColor = bMiningAvailable ? ActiveColor : DisabledColor;
	const FLinearColor PickupColor = bCarryFull
		? FullColor
		: (bPickupAvailable ? PickupActiveColor : DisabledColor);

	const auto DrawRect = [&](int32 Layer, const FVector2f& Position, const FVector2f& Size, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			Layer,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
			WhiteBrush,
			ESlateDrawEffect::None,
			Color);
	};

	DrawRect(BaseLayer + 1, FVector2f(0.0f, 0.0f), FVector2f(96.0f, 96.0f), MiningColor);
	DrawRect(BaseLayer + 2, FVector2f(3.0f, 3.0f), FVector2f(90.0f, 90.0f), FLinearColor(0.035f, 0.045f, 0.055f, 0.94f));

	// Simple side-profile drill: motor, barrel, bit, handle, and battery.
	DrawRect(BaseLayer + 3, FVector2f(17.0f, 23.0f), FVector2f(29.0f, 25.0f), MiningColor);
	DrawRect(BaseLayer + 3, FVector2f(45.0f, 29.0f), FVector2f(19.0f, 13.0f), MiningColor);
	DrawRect(BaseLayer + 3, FVector2f(64.0f, 33.0f), FVector2f(16.0f, 5.0f), MiningColor);
	DrawRect(BaseLayer + 3, FVector2f(80.0f, 31.0f), FVector2f(7.0f, 9.0f), MiningColor);
	DrawRect(BaseLayer + 3, FVector2f(29.0f, 47.0f), FVector2f(12.0f, 22.0f), MiningColor);
	DrawRect(BaseLayer + 3, FVector2f(24.0f, 67.0f), FVector2f(22.0f, 8.0f), MiningColor);

	constexpr float PickupSlotX = 108.0f;
	DrawRect(BaseLayer + 1, FVector2f(PickupSlotX, 0.0f), FVector2f(96.0f, 96.0f), PickupColor);
	DrawRect(BaseLayer + 2, FVector2f(PickupSlotX + 3.0f, 3.0f), FVector2f(90.0f, 90.0f), FLinearColor(0.035f, 0.045f, 0.055f, 0.94f));

	// Down arrow entering a cargo bin: the player's explicit pickup action.
	DrawRect(BaseLayer + 3, FVector2f(PickupSlotX + 44.0f, 17.0f), FVector2f(8.0f, 28.0f), PickupColor);
	DrawRect(BaseLayer + 3, FVector2f(PickupSlotX + 35.0f, 36.0f), FVector2f(26.0f, 8.0f), PickupColor);
	DrawRect(BaseLayer + 3, FVector2f(PickupSlotX + 22.0f, 53.0f), FVector2f(52.0f, 25.0f), PickupColor);
	DrawRect(BaseLayer + 4, FVector2f(PickupSlotX + 27.0f, 58.0f), FVector2f(42.0f, 15.0f), FLinearColor(0.035f, 0.045f, 0.055f, 0.94f));

	FSlateDrawElement::MakeText(
		OutDrawElements,
		BaseLayer + 4,
		AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(8.0f, 70.0f))),
		FText::FromString(TEXT("Q")),
		FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 16),
		ESlateDrawEffect::None,
		FLinearColor::White);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		BaseLayer + 5,
		AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(PickupSlotX + 8.0f, 70.0f))),
		FText::FromString(TEXT("R")),
		FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 16),
		ESlateDrawEffect::None,
		FLinearColor::White);

	return BaseLayer + 5;
}
