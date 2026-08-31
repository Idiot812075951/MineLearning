#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerTransformZone.h"
#include "MineLearningPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FTransformationSelectionVisibilityChangedSignature,
	bool,
	bVisible);

/** Persistent owner of transformation-selection input across pawn swaps. */
UCLASS()
class MINELEARNING_API AMineLearningPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Transformation")
	void ToggleTransformationSelection();

	UFUNCTION(BlueprintCallable, Category="Transformation")
	void SelectTransformationForm(EPlayerTransformationForm Form);

	UFUNCTION(BlueprintPure, Category="Transformation")
	bool IsTransformationSelectionOpen() const { return bTransformationSelectionOpen; }

	UPROPERTY(BlueprintAssignable, Category="Transformation")
	FTransformationSelectionVisibilityChangedSignature OnTransformationSelectionVisibilityChanged;

private:
	void SelectHumanForm();
	void SelectOreBuddyForm();
	void SelectGunnerForm();
	void SetTransformationSelectionOpen(bool bOpen);

	bool bTransformationSelectionOpen = false;
};
