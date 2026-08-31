#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CompanionBarkComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FCompanionBarkRequestedSignature,
	FText, Text,
	FLinearColor, Color,
	bool, bShowIcon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FCompanionBarkVisibilityChangedSignature,
	bool, bVisible);

UCLASS(ClassGroup=(Companion), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UCompanionBarkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCompanionBarkComponent();

	UFUNCTION(BlueprintCallable, Category="Companion|Bark")
	bool TrySpeak(
		const FText& Text,
		FLinearColor Color = FLinearColor::White,
		bool bShowIcon = false,
		bool bIgnoreCooldown = false);

	UPROPERTY(BlueprintAssignable, Category="Companion|Bark")
	FCompanionBarkRequestedSignature OnBarkRequested;

	UPROPERTY(BlueprintAssignable, Category="Companion|Bark")
	FCompanionBarkVisibilityChangedSignature OnBarkVisibilityChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Bark", meta=(ClampMin="0.1"))
	float SpeakDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Companion|Bark", meta=(ClampMin="0.0"))
	float SpeakCooldown = 3.0f;

private:
	void HideBark();
	double NextSpeakTime = 0.0;
	FTimerHandle HideTimerHandle;
};
