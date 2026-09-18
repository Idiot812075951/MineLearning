#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "GrabTargetIndicatorComponent.generated.h"

class UGurenQSkillComponent;
class UGrabbableComponent;
class UWidgetComponent;

/** Data passed by the UI host. The Widget Blueprint owns bindings, brush, tint and all visual responses. */
UCLASS(Abstract)
class MINELEARNING_API UGrabTargetWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "Grab Indicator") TObjectPtr<UGurenQSkillComponent> Source;
	UPROPERTY(BlueprintReadOnly, Category = "Grab Indicator") TObjectPtr<UGrabbableComponent> Target;
};

/** Owns only local screen-space widget lifetimes, subscribing to gameplay selection. */
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UGrabTargetIndicatorComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGrabTargetIndicatorComponent();
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, Category = "Grab Indicator") TSubclassOf<UGrabTargetWidget> IndicatorClass;
	UPROPERTY(EditAnywhere, Category = "Grab Indicator") FVector2D DrawSize = FVector2D(72.f, 72.f);
private:
	UPROPERTY() TObjectPtr<UGurenQSkillComponent> Source;
	UPROPERTY() TMap<TObjectPtr<UGrabbableComponent>, TObjectPtr<UWidgetComponent>> Indicators;
	UFUNCTION() void ReconcileIndicators();
};
