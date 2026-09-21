#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerTransformZone.h"
#include "Combat/CombatTypes.h"
#include "MineLearningPlayerController.generated.h"

class AWarehouseDepot;
class UUserWidget;
class UHealthComponent;
enum class EGurenQStage : uint8;

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
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	UFUNCTION(BlueprintCallable, Category = "Combat") void SelectCombatUnit(AActor* Actor);
	UFUNCTION(BlueprintCallable, Category = "Combat") void ToggleCombatDetails();
	UFUNCTION(BlueprintPure, Category = "Combat") bool IsCombatDetailsOpen() const { return bCombatDetailsOpen; }
	UFUNCTION(BlueprintPure, Category = "Combat") AActor* GetSelectedCombatUnit() const;
	UFUNCTION(BlueprintPure, Category = "Combat") FCombatPanelViewData GetCombatPanelData() const;
	UFUNCTION(BlueprintPure, Category = "Combat") FText GetCombatPanelText() const { return GetCombatPanelData().Details; }
	UFUNCTION(BlueprintPure, Category = "Combat") FText GetControlledSkillDescription(FName SkillId) const;
	UPROPERTY(BlueprintAssignable, Category = "Combat") FCombatStateChanged OnCombatInspectionChanged;
	UFUNCTION(Exec) void CombatDamage(float Amount = 100.f);
	UFUNCTION(Exec) void CombatHeal(float Amount = 100.f);
	UFUNCTION(Exec) void CombatSetHealth(float Health = 1000.f);
	UFUNCTION(Exec) void CombatSpawnDummy(float MaxHealth = 5000.f);

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
	void SelectCombatUnitUnderCursor();
	void UnbindCombatUnit();
	UFUNCTION() void CombatUnitChanged();
	UFUNCTION() void CombatUnitDestroyed(AActor* Actor);
	void ObserveQTarget();
	TWeakObjectPtr<UHealthComponent> ObservedQHealth;
	UFUNCTION() void CombatQStageChanged(EGurenQStage Stage, AActor* Target);
	UFUNCTION() void CombatAmmoChanged(int32 Ammo, int32 Maximum);
	UPROPERTY(EditDefaultsOnly, Category = "Combat|UI") TSoftClassPtr<UUserWidget> CombatWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Input") FKey CombatDetailsKey;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> CombatWidget;
	UPROPERTY(Transient) TObjectPtr<AActor> SelectedCombatUnit;
	bool bCombatDetailsOpen = false;
	void HandleInteraction();
	AWarehouseDepot* FindNearbyWarehouse() const;
	bool OpenWarehouseScreen(AWarehouseDepot* Warehouse);
	void RefreshMenuInputState();
	void SelectHumanForm();
	void SelectOreBuddyForm();
	void SelectGunnerForm();
	void SelectGurenForm();
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
