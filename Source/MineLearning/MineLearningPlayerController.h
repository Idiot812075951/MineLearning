#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerTransformZone.h"
#include "MineLearningPlayerController.generated.h"

class AWarehouseDepot;
class UUserWidget;

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
	AMineLearningPlayerController();

	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Transformation")
	void ToggleTransformationSelection();

	UFUNCTION(BlueprintCallable, Category="Transformation")
	void SelectTransformationForm(EPlayerTransformationForm Form);

	UFUNCTION(BlueprintPure, Category="Transformation")
	bool IsTransformationSelectionOpen() const { return bTransformationSelectionOpen; }

	UFUNCTION(BlueprintPure, Category="Warehouse")
	AWarehouseDepot* GetActiveWarehouse() const { return ActiveWarehouse; }

	UFUNCTION(BlueprintPure, Category="Warehouse")
	bool IsWarehouseScreenOpen() const { return bWarehouseScreenOpen; }

	UFUNCTION(BlueprintCallable, Category="Warehouse")
	void CloseWarehouseScreen();

	UPROPERTY(BlueprintAssignable, Category="Transformation")
	FTransformationSelectionVisibilityChangedSignature OnTransformationSelectionVisibilityChanged;

private:
	void HandleInteraction();
	AWarehouseDepot* FindNearbyWarehouse() const;
	bool OpenWarehouseScreen(AWarehouseDepot* Warehouse);
	void RefreshMenuInputState();
	void SelectHumanForm();
	void SelectOreBuddyForm();
	void SelectGunnerForm();
	void SetTransformationSelectionOpen(bool bOpen);

	UPROPERTY(EditDefaultsOnly, Category="Warehouse|UI")
	TSoftClassPtr<UUserWidget> WarehouseWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveWarehouseWidget;

	UPROPERTY(Transient)
	TObjectPtr<AWarehouseDepot> ActiveWarehouse;

	bool bWarehouseScreenOpen = false;
	bool bTransformationSelectionOpen = false;
};
