#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "MineLearning/Combat/UnitEffectDefinition.h"
#include "MineLearning/Manifestation/TransformationTypes.h"
#include "MineLearning/Mining/ItemTypes.h"
#include "RunContentCatalog.generated.h"

class APawn;
class UMaterialInterface;
class ARogueliteShop;

UENUM(BlueprintType)
enum class EUnlockKind : uint8 { Form, Upgrade, Summoner };

UENUM(BlueprintType)
enum class EUpgradeAudience : uint8 { ControlledPlayer, AllOwnedUnits };

USTRUCT(BlueprintType)
struct FUnlockGrant
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUnlockKind Kind = EUnlockKind::Upgrade;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EPlayerTransformationForm Form = EPlayerTransformationForm::Human;
};

USTRUCT(BlueprintType)
struct FTalentNodeRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Cost = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Prerequisites;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FUnlockGrant> Grants;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D Position = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UMaterialInterface> Icon;
};

USTRUCT(BlueprintType)
struct FSummonerRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UMaterialInterface> Icon;
};

USTRUCT(BlueprintType)
struct FUpgradeRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bInitiallyAvailable = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> RequiredSummoners;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EPlayerTransformationForm RequiredPurchasedForm = EPlayerTransformationForm::Human;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaxRank = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Weight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUpgradeAudience Audience = EUpgradeAudience::AllOwnedUnits;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TSoftClassPtr<APawn>> UnitClasses;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bIncludePhantoms = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TObjectPtr<UUnitEffectDefinition>> Effects;
	/** Application-layer capability; numeric effects never create pawns. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bGrantsPhantomCompanion = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Category;
	/** Short card copy. Full rules remain in Description for the tooltip. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText ShortDescription;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UMaterialInterface> Icon;
	/** Display-only audience badge; UnitClasses remains the gameplay filter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText UnitLabel;
};

USTRUCT(BlueprintType)
struct FFormPurchaseRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EPlayerTransformationForm Form = EPlayerTransformationForm::Human;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FItemStack> Cost;
};

UCLASS(BlueprintType)
class MINELEARNING_API URunContentCatalog : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UDataTable> Talents;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UDataTable> Summoners;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UDataTable> Upgrades;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UDataTable> Forms;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<EPlayerTransformationForm> DefaultForms = {EPlayerTransformationForm::Human, EPlayerTransformationForm::OreBuddy, EPlayerTransformationForm::Carrier, EPlayerTransformationForm::Gunner};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FItemStack> DraftCost;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 VictoryPoints = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SaveSlot = TEXT("TalentProfile_v2");
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UUnitEffectDefinition> PhantomHaste;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float PhantomLifetime = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UMaterialInterface> PhantomMaterial;
	/** Optional scene default. Authored level shops take precedence. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<ARogueliteShop> DefaultShopClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FTransform DefaultShopTransform;
	const FUpgradeRow* FindUpgrade(FName Id) const;
	const FTalentNodeRow* FindTalent(FName Id) const;
	UFUNCTION(BlueprintPure, Category="Content") TArray<FName> GetTalentIds() const { return Talents ? Talents->GetRowNames() : TArray<FName>(); }
	UFUNCTION(BlueprintPure, Category="Content") TArray<FName> GetSummonerIds() const { return Summoners ? Summoners->GetRowNames() : TArray<FName>(); }
	UFUNCTION(BlueprintPure, Category="Content") TArray<FName> GetUpgradeIds() const { return Upgrades ? Upgrades->GetRowNames() : TArray<FName>(); }
	UFUNCTION(BlueprintPure, Category="Content") FTalentNodeRow GetTalentData(FName Id) const;
	UFUNCTION(BlueprintPure, Category="Content") FUpgradeRow GetUpgradeData(FName Id) const;
	UFUNCTION(BlueprintPure, Category="Content") FSummonerRow GetSummonerData(FName Id) const;
	bool ValidateCatalog(FString& Error) const;
};
