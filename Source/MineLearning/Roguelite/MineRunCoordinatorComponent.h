#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RunContentCatalog.h"
#include "RunContentView.h"
#include "MineLearning/Combat/WeaponActionComponent.h"
#include "MineRunCoordinatorComponent.generated.h"

class UMetaProgressComponent;
class URunBuildComponent;
class UUpgradeDraftComponent;
class UPhantomCompanionComponent;
class UDemoRunComponent;
class APlayerController;
class APlayerTransformZone;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FControlledUnitReady, APawn*, Unit);

UCLASS()
class MINELEARNING_API UMineRunCoordinatorComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UMineRunCoordinatorComponent();
	UPROPERTY(EditDefaultsOnly, Category="Content") TSoftObjectPtr<URunContentCatalog> CatalogAsset;
	UFUNCTION(BlueprintPure, Category="Run") URunContentCatalog* GetCatalog() const { return Catalog; }
	UFUNCTION(BlueprintPure, Category="Run") UMetaProgressComponent* GetMetaProgress() const { return Meta; }
	UFUNCTION(BlueprintPure, Category="Run") URunBuildComponent* GetRunBuild() const { return Build; }
	UFUNCTION(BlueprintPure, Category="Run") UUpgradeDraftComponent* GetDraft() const { return Draft; }
	UFUNCTION(BlueprintPure, Category="Run") UPhantomCompanionComponent* GetPhantoms() const { return Phantoms; }
	UFUNCTION(BlueprintPure, Category="Run") FText GetFeedback() const { return Feedback; }
	UFUNCTION(BlueprintPure, Category="Talent") bool CanManageProfile() const;
	UFUNCTION(BlueprintPure, Category="Content") TArray<FRunContentView> GetUpgradeCards(bool bEligible) const;
	UFUNCTION(BlueprintPure, Category="Content") TArray<FRunContentView> GetOfferCards() const;
	UFUNCTION(BlueprintPure, Category="Content") TArray<FRunContentView> GetTalentNodes() const;
	UFUNCTION(BlueprintPure, Category="Content") TArray<FTalentLinkView> GetTalentLinks() const;
	UFUNCTION(BlueprintPure, Category="Content") TArray<FRunContentView> GetSummonerCards() const;
	UFUNCTION(BlueprintPure, Category="Draft") FText GetShopPriceText() const;
	UFUNCTION(BlueprintPure, Category="Draft") FText GetCoinBalanceText() const;
	UFUNCTION(BlueprintPure, Category="Draft") bool CanPurchaseDraft() const;
	UFUNCTION(BlueprintCallable, Category="Talent") bool Research(FName Node);
	UFUNCTION(BlueprintCallable, Category="Talent") bool SelectSummoner(FName Id);
	UFUNCTION(BlueprintCallable, Category="Draft") bool BuyDraft();
	UFUNCTION(BlueprintCallable, Category="Draft") bool ChooseUpgrade(int32 OfferId, FName Id);
	UFUNCTION(BlueprintCallable, Category="GM") void GrantDebugPoints(int32 Amount = 10);
	UFUNCTION(BlueprintCallable, Category="GM") void GrantDebugResources();
	UFUNCTION(BlueprintCallable, Category="GM") bool ClearProfileAndRestart();
	UFUNCTION(BlueprintCallable, Category="GM") bool GrantDebugUpgrade(FName Id);
	UFUNCTION(BlueprintCallable, Category="GM") void SetReloadChance(float Chance = -1.f);
	void AssembleUnit(APawn* Unit);
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnFeedbackChanged;
	/** Published after the controlled pawn's runtime components and grants are assembled. */
	UPROPERTY(BlueprintAssignable) FControlledUnitReady OnControlledUnitReady;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	FRunContentView MakeUpgradeCard(FName Id) const;
	void Initialize();
	bool PrepareRun();
	bool Respond(bool Success, const FText& Message);
	void RefreshUnits();
	void Transformed(APlayerController* Player, APawn* Unit, EPlayerTransformationForm Form);
	UFUNCTION() void PawnChanged(APawn* OldPawn, APawn* NewPawn);
	UFUNCTION() void BuildChanged();
	UFUNCTION() void RunChanged();
	UFUNCTION() void CompanionChanged();
	UFUNCTION() void PickupCompleted(AActor* Collector, EItemType Item, int32 Amount);
	UPROPERTY(Transient) TObjectPtr<URunContentCatalog> Catalog;
	UPROPERTY(Transient) TObjectPtr<UMetaProgressComponent> Meta;
	UPROPERTY(Transient) TObjectPtr<URunBuildComponent> Build;
	UPROPERTY(Transient) TObjectPtr<UUpgradeDraftComponent> Draft;
	UPROPERTY(Transient) TObjectPtr<UPhantomCompanionComponent> Phantoms;
	UPROPERTY(Transient) TObjectPtr<UDemoRunComponent> Run;
	TArray<TWeakObjectPtr<APawn>> Units;
	TMap<TWeakObjectPtr<APlayerTransformZone>, FDelegateHandle> TransformationBindings;
	FDelegateHandle UnitReadyHandle;
	FWeaponRuntimeState SavedPlayerWeapon;
	bool bHasSavedPlayerWeapon = false;
	bool bReady = false;
	bool bHandlingRun = false;
	bool bPhantomAbilityActive = false;
	FText Feedback;
};
