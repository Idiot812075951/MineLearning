#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RunContentCatalog.h"
#include "UpgradeDraftComponent.generated.h"

class URunBuildComponent;
class UResourceStorageComponent;
DECLARE_DELEGATE_RetVal_OneParam(bool, FFormPurchasedQuery, EPlayerTransformationForm);

UCLASS()
class MINELEARNING_API UUpgradeDraftComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUpgradeDraftComponent();
	void Initialize(URunContentCatalog* Config, URunBuildComponent* CurrentBuild, UResourceStorageComponent* Account, FFormPurchasedQuery FormQuery);
	bool BuyOffer();
	bool Choose(int32 OfferId, FName Upgrade);
	void ResetOffer();
	UFUNCTION(BlueprintPure, Category="Draft") TArray<FItemStack> GetNextCost() const;
	UFUNCTION(BlueprintPure, Category="Draft") FText GetEligibilityBlock(FName Upgrade) const;
	UFUNCTION(BlueprintPure, Category="Draft") bool CanBuyOffer() const;
	UFUNCTION(BlueprintPure, Category="Draft") TArray<FName> GetCandidates() const { return Candidates; }
	UFUNCTION(BlueprintPure, Category="Draft") int32 GetOfferId() const { return CurrentOfferId; }
	UFUNCTION(BlueprintPure, Category="Draft") bool HasPendingOffer() const { return !Candidates.IsEmpty(); }
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnOfferChanged;
private:
	UPROPERTY(Transient) TObjectPtr<URunContentCatalog> Catalog;
	UPROPERTY(Transient) TObjectPtr<URunBuildComponent> Build;
	UPROPERTY(Transient) TObjectPtr<UResourceStorageComponent> Wallet;
	FFormPurchasedQuery IsFormPurchased;
	TArray<FName> Candidates;
	FRandomStream Random;
	int32 CurrentOfferId = 0;
	int32 PaidOfferCount = 0;
	bool bCommitting = false;
};
