#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "GunnerHitFeedbackComponent.generated.h"

class AGunnerCharacter;
class UWidgetComponent;

/** Immutable input for the Widget Blueprint's initial presentation. */
UCLASS(Abstract)
class MINELEARNING_API UGunnerHitFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "Hit Feedback") bool bGoldenHit = false;
};

/** Local world-widget lifetime and attachment; the WBP owns the badge appearance. */
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UGunnerHitFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGunnerHitFeedbackComponent();
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, Category = "Hit Feedback") TSubclassOf<UGunnerHitFeedbackWidget> FeedbackClass;
	UPROPERTY(EditAnywhere, Category = "Hit Feedback") FVector2D DrawSize = FVector2D(96.f, 96.f);
	UPROPERTY(EditAnywhere, Category = "Hit Feedback") float HeightOffset = 24.f;
	UPROPERTY(EditAnywhere, Category = "Hit Feedback", meta = (ClampMin = "0.1")) float Duration = 0.75f;
private:
	UPROPERTY() TObjectPtr<AGunnerCharacter> Source;
	UPROPERTY() TObjectPtr<UWidgetComponent> ActiveFeedback;
	FTimerHandle ExpiryHandle;
	UFUNCTION() void HandleCriticalHit(bool bGolden, AActor* Target, FVector TargetTop);
	void ClearFeedback();
};
