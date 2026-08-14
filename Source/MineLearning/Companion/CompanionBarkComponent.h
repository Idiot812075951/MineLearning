#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CompanionBarkComponent.generated.h"

class UWidgetComponent;
class UUserWidget;

UCLASS(ClassGroup=(Companion), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UCompanionBarkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCompanionBarkComponent();
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Companion|Bark")
	bool TrySpeak(
		const FText& Text,
		FLinearColor Color = FLinearColor::White,
		bool bShowIcon = false,
		bool bIgnoreCooldown = false);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Bark")
	TSubclassOf<UUserWidget> WidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Bark", meta=(ClampMin="0.1"))
	float SpeakDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Bark", meta=(ClampMin="0.0"))
	float SpeakCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Bark")
	// Tuned against the scaled Gunner BP in PIE. Its visual mesh is much smaller
	// than the character capsule, so a large generic character offset floats far
	// above it; 20 cm places the screen-space callout just above the visible head.
	FVector RelativeLocation = FVector(0.0f, 0.0f, 20.0f);

private:
	void HideBark();
	TObjectPtr<UWidgetComponent> BarkWidgetComponent;
	double NextSpeakTime = 0.0;
	FTimerHandle HideTimerHandle;
};
