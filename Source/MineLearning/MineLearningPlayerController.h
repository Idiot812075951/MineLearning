#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerTransformZone.h"
#include "Combat/CombatTypes.h"
#include "TransformationGuard.h"
#include "MineLearningPlayerController.generated.h"

class AWarehouseDepot;
class UUserWidget;
class UHealthComponent;
class UDemoRunComponent;
class ADemoRouteGuide;
class UMineRunCoordinatorComponent;
class ARogueliteShop;
class UInputAction;
class UInputMappingContext;
enum class EDemoCommand : uint8;
enum class EGurenQStage : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractionFeedbackSignature, FText, Message);

UENUM(BlueprintType)
enum class ERoguelitePage : uint8
{
	Preparation,
	Shop,
	Draft,
	Talents,
	Codex,
	Debug
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FTransformationSelectionVisibilityChangedSignature,
	bool,
	bVisible);

/** Persistent owner of transformation-selection input across pawn swaps. */
UCLASS()
class MINELEARNING_API AMineLearningPlayerController : public APlayerController, public ITransformationGuard
{
	GENERATED_BODY()

public:
	AMineLearningPlayerController();
	virtual bool CanTransform() const override;
	void ActivateSummonerAbility();
	void CycleContextTarget();
	UFUNCTION(BlueprintPure, Category="Roguelite") UMineRunCoordinatorComponent* GetRunCoordinator() const { return RunCoordinator; }
	UFUNCTION(BlueprintCallable, Category="Roguelite") void ToggleRogueliteMenu();
	UFUNCTION(BlueprintCallable, Category="Roguelite") void OpenRoguelitePage(ERoguelitePage Page);
	UFUNCTION(BlueprintCallable, Category="Roguelite") void CloseRogueliteMenu();
	UFUNCTION(BlueprintPure, Category="Roguelite") ERoguelitePage GetRoguelitePage() const { return RoguelitePage; }
	UFUNCTION(BlueprintPure, Category="Roguelite") bool IsNearRogueliteShop() const;
	UFUNCTION(BlueprintPure, Category="Roguelite") bool IsRogueliteMenuOpen() const { return bRogueliteMenuOpen; }
	UPROPERTY(BlueprintAssignable, Category="Roguelite") FCombatStateChanged OnRogueliteMenuChanged;
	UFUNCTION(BlueprintPure, Category="Demo") UDemoRunComponent* GetDemoRun() const { return DemoRun; }
	UFUNCTION(BlueprintCallable, Category="Demo") void ToggleDemoTerminal();
	UFUNCTION(BlueprintCallable, Category="Demo") void ExecuteDemoCommand(EDemoCommand Command);
	UFUNCTION(BlueprintPure, Category="Demo") bool IsDemoTerminalOpen() const { return bDemoTerminalOpen; }
	UPROPERTY(BlueprintAssignable, Category="Demo") FCombatStateChanged OnDemoTerminalChanged;
	UPROPERTY(BlueprintAssignable, Category="Interaction") FInteractionFeedbackSignature OnInteractionFeedback;
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
	UFUNCTION(Exec) void CombatSelectNearestOre();
	UFUNCTION(Exec) void CombatSpawnDummy(float MaxHealth = 5000.f);
	UFUNCTION(Exec, BlueprintCallable, Category="GM") void BuffAdd(FName Id, float Duration = -1.f);
	UFUNCTION(Exec) void BuffRemove(FName Id);
	UFUNCTION(Exec, BlueprintCallable, Category="GM") void BuffClear();
	UFUNCTION(Exec) void BuffList();
	/** GM acquisition through the real run build; still enforces talent/identity eligibility. */
	UFUNCTION(Exec) void UpgradeAdd(FName Id);

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
	UPROPERTY(EditDefaultsOnly, Category="Input|Run") TSoftObjectPtr<UInputAction> SummonerAbilityAction;
	UPROPERTY(EditDefaultsOnly, Category="Input|Run") TSoftObjectPtr<UInputAction> CycleContextAction;
	UPROPERTY(EditDefaultsOnly, Category="Input|Run") TSoftObjectPtr<UInputMappingContext> RunAbilityMapping;
	UPROPERTY(VisibleAnywhere, Category="Roguelite") TObjectPtr<UMineRunCoordinatorComponent> RunCoordinator;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> RogueliteWidget;
	UPROPERTY(EditDefaultsOnly, Category="UI") TSoftClassPtr<UUserWidget> RunMenuWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/MineLearning/GameplayRuntime/UI/WBP_RogueliteHub.WBP_RogueliteHub_C")));
	bool bRogueliteMenuOpen = false;
	ERoguelitePage RoguelitePage = ERoguelitePage::Preparation;
	TArray<TWeakObjectPtr<ARogueliteShop>> ObservedShops;
	UFUNCTION() void ShopRangeChanged(APawn* ChangedPawn, bool bNearby);
	UFUNCTION() void DraftChanged();
	void OpenTalents();
	void OpenCodex();
	UPROPERTY(VisibleAnywhere, Category="Demo") TObjectPtr<UDemoRunComponent> DemoRun;
	UPROPERTY(EditDefaultsOnly, Category="Demo|UI") TSoftClassPtr<UUserWidget> DemoWidgetClass;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> DemoWidget;
	UPROPERTY(Transient) TObjectPtr<ADemoRouteGuide> DemoRoute;
	bool bDemoTerminalOpen = false;
	bool bDemoResultPresented = false;
	UFUNCTION() void DemoRunChanged();
	void CloseMenus();
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
	void IgnorePawnCameraCollision(AActor* Actor);
	FDelegateHandle PawnCameraSpawnHandle;
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
