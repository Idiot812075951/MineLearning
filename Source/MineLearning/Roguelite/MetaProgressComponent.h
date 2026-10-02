#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SaveGame.h"
#include "MineLearning/Combat/CombatTypes.h"
#include "MetaProgressComponent.generated.h"

class URunContentCatalog;

UCLASS()
class MINELEARNING_API UTalentProfileSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame) int32 Version = 2;
	UPROPERTY(SaveGame) int32 TalentPoints = 0;
	UPROPERTY(SaveGame) TSet<FName> Researched;
	UPROPERTY(SaveGame) FName SelectedSummoner;
	UPROPERTY(SaveGame) FGuid LastRewardedRun;
};

UCLASS()
class MINELEARNING_API UMetaProgressComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UMetaProgressComponent();
	bool Initialize(URunContentCatalog* Config);
	bool Research(FName Node);
	UFUNCTION(BlueprintPure, Category="Talent") bool CanResearch(FName Node) const;
	bool SelectSummoner(FName Id);
	bool AwardVictory(FGuid RunId);
	bool AddDebugPoints(int32 Amount);
	bool ClearProfile();
	UFUNCTION(BlueprintPure, Category="Talent") int32 GetTalentPoints() const;
	UFUNCTION(BlueprintPure, Category="Talent") bool IsResearched(FName Node) const;
	UFUNCTION(BlueprintPure, Category="Talent") bool IsSummonerUnlocked(FName Id) const;
	UFUNCTION(BlueprintPure, Category="Talent") FName GetSelectedSummoner() const;
	const TSet<FName>& GetResearchedNodes() const;
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnProfileChanged;
private:
	bool SaveCandidate(UTalentProfileSaveGame* Candidate);
	UPROPERTY(Transient) TObjectPtr<UTalentProfileSaveGame> Profile;
	UPROPERTY(Transient) TObjectPtr<URunContentCatalog> Catalog;
};
