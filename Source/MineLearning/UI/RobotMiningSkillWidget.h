#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RobotMiningSkillWidget.generated.h"

/** Minimal player-OreBuddy skill bar rendered without a Blueprint widget asset. */
UCLASS()
class MINELEARNING_API URobotMiningSkillWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMiningAvailable(bool bAvailable);
	void SetPickupState(bool bAvailable, bool bCarryFull);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	bool bMiningAvailable = false;
	bool bPickupAvailable = false;
	bool bCarryFull = false;
};
