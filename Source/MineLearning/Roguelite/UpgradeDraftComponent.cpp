#include "UpgradeDraftComponent.h"
#include "RunBuildComponent.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"

UUpgradeDraftComponent::UUpgradeDraftComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUpgradeDraftComponent::Initialize(URunContentCatalog* Config, URunBuildComponent* CurrentBuild, UResourceStorageComponent* Account, FFormPurchasedQuery FormQuery)
{
	Catalog = Config;
	Build = CurrentBuild;
	Wallet = Account;
	IsFormPurchased = FormQuery;
	Random.Initialize(FGuid::NewGuid().A);
	PaidOfferCount = 0;
	ResetOffer();
}

bool UUpgradeDraftComponent::BuyOffer()
{
	if (bCommitting || !Catalog || !Build || !Build->IsRunActive() || !Wallet || HasPendingOffer())
	{
		return false;
	}
	TGuardValue<bool> Guard(bCommitting, true);
	TArray<FName> Pool;
	for (FName Id : Catalog->Upgrades->GetRowNames())
	{
		const FUpgradeRow* Row = Catalog->FindUpgrade(Id);
		if (!Row)
		{
			continue;
		}
		if (GetEligibilityBlock(Id).IsEmpty())
		{
			Pool.Add(Id);
		}
	}
	if (Pool.IsEmpty()) { return false; }
	Pool.Sort(FNameLexicalLess());
	TArray<FName> Prepared;
	while (!Pool.IsEmpty() && Prepared.Num() < 3)
	{
		float Total = 0.f;
		for (FName Id : Pool) { Total += Catalog->FindUpgrade(Id)->Weight; }
		float Roll = Random.FRand() * Total;
		int32 Selected = Pool.Num() - 1;
		for (int32 Index = 0; Index < Pool.Num(); ++Index)
		{
			Roll -= Catalog->FindUpgrade(Pool[Index])->Weight;
			if (Roll < 0.f) { Selected = Index; break; }
		}
		Prepared.Add(Pool[Selected]);
		Pool.RemoveAt(Selected);
	}
	const TArray<FItemStack> Cost = GetNextCost();
	if (Cost.IsEmpty() || !Wallet->TrySpendItems(Cost)) { return false; }
	Candidates = MoveTemp(Prepared);
	++PaidOfferCount;
	++CurrentOfferId;
	OnOfferChanged.Broadcast();
	return true;
}

bool UUpgradeDraftComponent::Choose(int32 OfferId, FName Upgrade)
{
	if (bCommitting || !Build || !Build->IsRunActive() || OfferId != CurrentOfferId
		|| !Candidates.Contains(Upgrade) || !GetEligibilityBlock(Upgrade).IsEmpty())
	{
		return false;
	}
	TGuardValue<bool> Guard(bCommitting, true);
	// Offer is consumed before the build event can reenter the command.
	TArray<FName> Previous = MoveTemp(Candidates);
	if (!Build->Acquire(Upgrade))
	{
		Candidates = MoveTemp(Previous);
		return false;
	}
	OnOfferChanged.Broadcast();
	return true;
}

void UUpgradeDraftComponent::ResetOffer()
{
	Candidates.Reset();
	++CurrentOfferId;
	OnOfferChanged.Broadcast();
}

TArray<FItemStack> UUpgradeDraftComponent::GetNextCost() const
{
	TArray<FItemStack> Result = Catalog ? Catalog->DraftCost : TArray<FItemStack>();
	for (FItemStack& Cost : Result)
	{
		const int64 Amount = static_cast<int64>(Cost.Amount) * (static_cast<int64>(PaidOfferCount) + 1);
		if (Amount <= 0 || Amount > MAX_int32)
		{
			return {};
		}
		Cost.Amount = static_cast<int32>(Amount);
	}
	return Result;
}

FText UUpgradeDraftComponent::GetEligibilityBlock(FName Upgrade) const
{
	if (!Build || !Catalog) { return NSLOCTEXT("Draft", "NotReady", "尚未就绪"); }
	const FText Block = Build->GetAcquisitionBlock(Upgrade);
	if (!Block.IsEmpty()) { return Block; }
	const FUpgradeRow* Row = Catalog->FindUpgrade(Upgrade);
	if (Row->RequiredPurchasedForm != EPlayerTransformationForm::Human
		&& (!IsFormPurchased.IsBound() || !IsFormPurchased.Execute(Row->RequiredPurchasedForm)))
	{
		return NSLOCTEXT("Draft", "FormLocked", "先购买对应幻化");
	}
	return FText::GetEmpty();
}

bool UUpgradeDraftComponent::CanBuyOffer() const
{
	if (bCommitting || !Catalog || !Build || !Build->IsRunActive() || !Wallet || HasPendingOffer()) { return false; }
	const TArray<FItemStack> Cost = GetNextCost();
	if (Cost.IsEmpty()) { return false; }
	for (const FItemStack& Item : Cost)
	{
		if (Wallet->GetAvailableItemAmount(Item.ItemType) < Item.Amount) { return false; }
	}
	for (FName Id : Catalog->GetUpgradeIds())
	{
		if (GetEligibilityBlock(Id).IsEmpty()) { return true; }
	}
	return false;
}
