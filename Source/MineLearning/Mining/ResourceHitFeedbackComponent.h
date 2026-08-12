#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceHitFeedbackComponent.generated.h"

class UNiagaraSystem;

UCLASS(ClassGroup=(Mining), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UResourceHitFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceHitFeedbackComponent();

	UFUNCTION(BlueprintCallable, Category="Resource|Feedback")
	void PlayHitFeedback(FVector Location, FVector Normal, float Strength);

	UFUNCTION(BlueprintCallable, Category="Resource|Feedback")
	void PlayDestroyedFeedback(FVector Location, FVector Normal);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resource|Feedback")
	TObjectPtr<UNiagaraSystem> HitSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resource|Feedback")
	TObjectPtr<UNiagaraSystem> DestroyedSystem;
};
