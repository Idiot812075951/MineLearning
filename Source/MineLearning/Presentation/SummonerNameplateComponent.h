#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SummonerNameplateComponent.generated.h"

class UUserWidget;
class UWidgetComponent;
class APawn;

/** Local UI host: owns only the attachment lifecycle, never summoner gameplay state. */
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API USummonerNameplateComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	USummonerNameplateComponent();
	UPROPERTY(EditDefaultsOnly, Category="UI") TSoftClassPtr<APawn> HumanPawnClass;
	UPROPERTY(EditDefaultsOnly, Category="UI") TSoftClassPtr<UUserWidget> WidgetClass;
	UPROPERTY(EditDefaultsOnly, Category="UI") FVector HeadOffset = FVector(0.f, 0.f, 125.f);
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION() void PawnChanged(APawn* OldPawn, APawn* NewPawn);
	void ReleaseNameplate();
	UPROPERTY(Transient) TObjectPtr<UWidgetComponent> Nameplate;
};
