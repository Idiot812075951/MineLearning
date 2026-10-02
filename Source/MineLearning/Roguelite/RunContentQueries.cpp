#include "MineRunCoordinatorComponent.h"
#include "MetaProgressComponent.h"
#include "RunBuildComponent.h"
#include "UpgradeDraftComponent.h"
#include "RogueliteShop.h"
#include "MineLearning/Demo/DemoRunComponent.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "RunContentQueries"

FRunContentView UMineRunCoordinatorComponent::MakeUpgradeCard(FName Id) const
{
	FRunContentView View;
	const FUpgradeRow* Row = Catalog ? Catalog->FindUpgrade(Id) : nullptr;
	if (!Row)
	{
		return View;
	}
	View.Id = Id;
	View.Title = Row->DisplayName;
	View.Summary = Row->ShortDescription;
	View.Details = Row->Description;
	for (const UUnitEffectDefinition* Effect : Row->Effects)
	{
		if (Effect && !Effect->GetRuleDescription().IsEmpty())
		{
			View.Details = FText::Format(LOCTEXT("ConfiguredRules", "{0}\n{1}"), View.Details, Effect->GetRuleDescription());
		}
	}
	View.Category = Row->Category;
	View.Icon = Row->Icon;
	View.UnitLabel = Row->UnitLabel;
	View.State = Draft ? Draft->GetEligibilityBlock(Id) : LOCTEXT("NotReady", "尚未开局");
	View.bEnabled = View.State.IsEmpty();
	if (View.bEnabled)
	{
		View.State = FText::Format(LOCTEXT("Rank", "本局可抽 · {0}/{1}"), Build->GetUpgradeRank(Id), Row->MaxRank);
	}
	return View;
}

TArray<FRunContentView> UMineRunCoordinatorComponent::GetUpgradeCards(bool bEligible) const
{
	TArray<FRunContentView> Views;
	if (!Catalog)
	{
		return Views;
	}
	TArray<FName> Ids = Catalog->GetUpgradeIds();
	Ids.Sort(FNameLexicalLess());
	for (FName Id : Ids)
	{
		FRunContentView View = MakeUpgradeCard(Id);
		if (View.bEnabled == bEligible)
		{
			Views.Add(MoveTemp(View));
		}
	}
	return Views;
}

TArray<FRunContentView> UMineRunCoordinatorComponent::GetOfferCards() const
{
	TArray<FRunContentView> Views;
	if (Draft)
	{
		for (FName Id : Draft->GetCandidates())
		{
			FRunContentView View = MakeUpgradeCard(Id);
			View.State = LOCTEXT("Choose", "选择升级");
			Views.Add(MoveTemp(View));
		}
	}
	return Views;
}

TArray<FRunContentView> UMineRunCoordinatorComponent::GetTalentNodes() const
{
	TArray<FRunContentView> Views;
	if (!Catalog || !Meta)
	{
		return Views;
	}
	for (FName Id : Catalog->GetTalentIds())
	{
		const FTalentNodeRow* Row = Catalog->FindTalent(Id);
		FRunContentView View;
		View.Id = Id;
		View.Title = Row->DisplayName;
		View.Details = Row->Description;
		View.Icon = Row->Icon;
		View.Position = Row->Position;
		View.bEnabled = CanManageProfile() && Meta->CanResearch(Id);
		if (Meta->IsResearched(Id))
		{
			View.State = LOCTEXT("Researched", "已解锁");
		}
		else if (!CanManageProfile())
		{
			View.State = LOCTEXT("NextRun", "局间解锁");
		}
		else if (Meta->CanResearch(Id))
		{
			View.State = FText::Format(LOCTEXT("Unlock", "解锁 · {0} 点"), Row->Cost);
		}
		else
		{
			bool bMissingParent = false;
			for (FName Parent : Row->Prerequisites)
			{
				bMissingParent |= !Meta->IsResearched(Parent);
			}
			View.State = bMissingParent ? LOCTEXT("Parent", "前置未解锁") : FText::Format(LOCTEXT("NeedPoints", "需要 {0} 点"), Row->Cost);
		}
		Views.Add(MoveTemp(View));
	}
	return Views;
}

TArray<FTalentLinkView> UMineRunCoordinatorComponent::GetTalentLinks() const
{
	TArray<FTalentLinkView> Links;
	if (!Catalog)
	{
		return Links;
	}
	for (FName Id : Catalog->GetTalentIds())
	{
		const FTalentNodeRow* Row = Catalog->FindTalent(Id);
		for (FName Parent : Row->Prerequisites)
		{
			if (const FTalentNodeRow* From = Catalog->FindTalent(Parent))
			{
				FTalentLinkView Link;
				Link.Start = From->Position;
				Link.End = Row->Position;
				Links.Add(Link);
			}
		}
	}
	return Links;
}

TArray<FRunContentView> UMineRunCoordinatorComponent::GetSummonerCards() const
{
	TArray<FRunContentView> Views;
	if (!Catalog || !Meta)
	{
		return Views;
	}
	FRunContentView Neutral;
	Neutral.Title = LOCTEXT("Neutral", "自由矿工");
	Neutral.Summary = LOCTEXT("NeutralSummary", "通用升级池");
	Neutral.bEnabled = CanManageProfile();
	Views.Add(Neutral);
	for (FName Id : Catalog->GetSummonerIds())
	{
		const FSummonerRow Row = Catalog->GetSummonerData(Id);
		FRunContentView View;
		View.Id = Id;
		View.Title = Row.DisplayName;
		View.Summary = Row.Description;
		View.Icon = Row.Icon;
		View.bEnabled = CanManageProfile() && Meta->IsSummonerUnlocked(Id);
		Views.Add(View);
	}
	const FName Identity = Build && Build->IsRunActive() ? Build->GetSummoner() : Meta->GetSelectedSummoner();
	for (FRunContentView& View : Views)
	{
		View.Category = LOCTEXT("Identity", "召唤师");
		if (View.Id == Identity)
		{
			View.State = LOCTEXT("Selected", "已选择");
		}
		else if (!CanManageProfile())
		{
			View.State = LOCTEXT("IdentityFrozen", "本局身份已锁定");
		}
		else
		{
			View.State = View.bEnabled ? LOCTEXT("Select", "免费选择") : LOCTEXT("Locked", "天赋解锁");
		}
	}
	return Views;
}

FText UMineRunCoordinatorComponent::GetShopPriceText() const
{
	if (Draft && Draft->HasPendingOffer())
	{
		return LOCTEXT("Continue", "继续选择 · 已支付");
	}
	const TArray<FItemStack> Cost = Draft ? Draft->GetNextCost() : TArray<FItemStack>();
	if (Cost.Num() == 1 && Cost[0].ItemType == EItemType::Coin)
	{
		return FText::Format(LOCTEXT("Price", "三选一 · {0} 金币"), Cost[0].Amount);
	}
	return LOCTEXT("Unavailable", "暂不可购买");
}

FText UMineRunCoordinatorComponent::GetCoinBalanceText() const
{
	const AWarehouseDepot* Warehouse = Run ? Run->GetWarehouse() : nullptr;
	const int32 Coins = Warehouse ? Warehouse->GetStorageComponent()->GetAvailableItemAmount(EItemType::Coin) : 0;
	return FText::Format(LOCTEXT("Coins", "仓库金币  {0}"), Coins);
}

bool UMineRunCoordinatorComponent::CanPurchaseDraft() const
{
	const APlayerController* Player = Cast<APlayerController>(GetOwner());
	return bReady && Player && Player->HasAuthority() && ARogueliteShop::FindNearby(Player->GetPawn()) && Draft && Draft->CanBuyOffer();
}

#undef LOCTEXT_NAMESPACE
