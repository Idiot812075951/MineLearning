#include "RunContentCatalog.h"
#include "GameFramework/Pawn.h"

FTalentNodeRow URunContentCatalog::GetTalentData(FName Id) const
{
	const FTalentNodeRow* Row = FindTalent(Id);
	return Row ? *Row : FTalentNodeRow();
}

FUpgradeRow URunContentCatalog::GetUpgradeData(FName Id) const
{
	const FUpgradeRow* Row = FindUpgrade(Id);
	return Row ? *Row : FUpgradeRow();
}

FSummonerRow URunContentCatalog::GetSummonerData(FName Id) const
{
	const FSummonerRow* Row = Summoners ? Summoners->FindRow<FSummonerRow>(Id, TEXT("Summoner"), false) : nullptr;
	return Row ? *Row : FSummonerRow();
}

const FUpgradeRow* URunContentCatalog::FindUpgrade(FName Id) const
{
	return Upgrades ? Upgrades->FindRow<FUpgradeRow>(Id, TEXT("Upgrade"), false) : nullptr;
}

const FTalentNodeRow* URunContentCatalog::FindTalent(FName Id) const
{
	return Talents ? Talents->FindRow<FTalentNodeRow>(Id, TEXT("Talent"), false) : nullptr;
}

bool URunContentCatalog::ValidateCatalog(FString& Error) const
{
	if (!Talents || Talents->GetRowStruct() != FTalentNodeRow::StaticStruct()
		|| !Summoners || Summoners->GetRowStruct() != FSummonerRow::StaticStruct()
		|| !Upgrades || Upgrades->GetRowStruct() != FUpgradeRow::StaticStruct()
		|| !Forms || Forms->GetRowStruct() != FFormPurchaseRow::StaticStruct()
		|| DraftCost.IsEmpty() || SaveSlot.IsEmpty() || VictoryPoints < 0 || PhantomLifetime <= 0.f)
	{
		Error = TEXT("Missing tables, invalid row types, or invalid run configuration.");
		return false;
	}
	for (const FItemStack& Cost : DraftCost)
	{
		if (Cost.Amount <= 0) { Error = TEXT("Draft cost must be positive."); return false; }
	}
	TSet<FName> Visiting;
	TSet<FName> Complete;
	TFunction<bool(FName)> Visit = [&](FName Id)
	{
		if (Complete.Contains(Id)) { return true; }
		const FTalentNodeRow* Node = FindTalent(Id);
		if (!Node || Node->Cost < 1 || Visiting.Contains(Id)) { return false; }
		Visiting.Add(Id);
		for (FName Parent : Node->Prerequisites)
		{
			if (!Visit(Parent)) { return false; }
		}
		for (const FUnlockGrant& Grant : Node->Grants)
		{
			if (Grant.Kind == EUnlockKind::Upgrade && !FindUpgrade(Grant.Id)) { return false; }
			if (Grant.Kind == EUnlockKind::Summoner && !Summoners->FindRow<FSummonerRow>(Grant.Id, TEXT("Validate"), false)) { return false; }
		}
		Visiting.Remove(Id);
		Complete.Add(Id);
		return true;
	};
	for (FName Id : Talents->GetRowNames())
	{
		if (!Visit(Id)) { Error = TEXT("Invalid talent graph: ") + Id.ToString(); return false; }
	}
	for (FName Id : Upgrades->GetRowNames())
	{
		const FUpgradeRow* Row = FindUpgrade(Id);
		if (Row->MaxRank < 1 || !FMath::IsFinite(Row->Weight) || Row->Weight <= 0.f
			|| (Row->Effects.IsEmpty() && !Row->bGrantsPhantomCompanion))
		{
			Error = TEXT("Invalid upgrade: ") + Id.ToString(); return false;
		}
		for (UUnitEffectDefinition* Effect : Row->Effects)
		{
			if (!Effect || !Effect->ValidateDefinition(Error))
			{
				Error = TEXT("Invalid effect in upgrade ") + Id.ToString() + TEXT(": ") + Error;
				return false;
			}
			for (const TSoftClassPtr<APawn>& UnitType : Row->UnitClasses)
			{
				UClass* UnitClass = UnitType.LoadSynchronous();
				if (!UnitClass || !Effect->SupportsTarget(UnitClass->GetDefaultObject<AActor>()))
				{
					Error = TEXT("Effect does not support configured unit class: ") + Id.ToString();
					return false;
				}
			}
		}
		for (FName Identity : Row->RequiredSummoners)
		{
			if (!Summoners->FindRow<FSummonerRow>(Identity, TEXT("Validate"), false)) { return false; }
		}
	}
	for (FName Id : Forms->GetRowNames())
	{
		const FFormPurchaseRow* Row = Forms->FindRow<FFormPurchaseRow>(Id, TEXT("Validate"));
		if (Row->Cost.IsEmpty()) { Error = TEXT("Form purchase requires a cost."); return false; }
		for (const FItemStack& Cost : Row->Cost)
		{
			if (!Cost.IsValid()) { Error = TEXT("Invalid form purchase cost."); return false; }
		}
	}
	if (!PhantomHaste || !PhantomHaste->ValidateDefinition(Error) || PhantomMaterial.IsNull())
	{
		Error = TEXT("Missing phantom effect or material.");
		return false;
	}
	return true;
}
