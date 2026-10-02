#include "RunBuildComponent.h"

URunBuildComponent::URunBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URunBuildComponent::BeginRun(URunContentCatalog* Config, const TSet<FName>& Nodes, FName Identity)
{
	Catalog = Config;
	LockedTalents = Nodes;
	Summoner = Identity;
	Owned.Reset();
	AvailableUpgrades.Reset();
	AvailableForms.Reset();
	for (EPlayerTransformationForm Form : Config->DefaultForms) { AvailableForms.Add(Form); }
	for (FName Node : Nodes)
	{
		const FTalentNodeRow* Definition = Config->FindTalent(Node);
		if (!Definition) { continue; }
		for (const FUnlockGrant& Grant : Definition->Grants)
		{
			if (Grant.Kind == EUnlockKind::Upgrade) { AvailableUpgrades.Add(Grant.Id); }
			if (Grant.Kind == EUnlockKind::Form) { AvailableForms.Add(Grant.Form); }
		}
	}
	RunId = FGuid::NewGuid();
	bActive = true;
	OnBuildChanged.Broadcast();
}

void URunBuildComponent::EndRun()
{
	bActive = false;
	OnBuildChanged.Broadcast();
}

bool URunBuildComponent::CanAcquire(FName Upgrade) const
{
	return GetAcquisitionBlock(Upgrade).IsEmpty();
}

FText URunBuildComponent::GetAcquisitionBlock(FName Upgrade) const
{
	const FUpgradeRow* Row = Catalog ? Catalog->FindUpgrade(Upgrade) : nullptr;
	if (!Row) { return NSLOCTEXT("Draft", "Missing", "配置缺失"); }
	if (!bActive) { return NSLOCTEXT("Draft", "Inactive", "尚未开局"); }
	if (!Row->bInitiallyAvailable && !AvailableUpgrades.Contains(Upgrade)) { return NSLOCTEXT("Draft", "TalentLocked", "本局天赋未解锁"); }
	if (!Row->RequiredSummoners.IsEmpty() && !Row->RequiredSummoners.Contains(Summoner)) { return NSLOCTEXT("Draft", "IdentityLocked", "需要对应召唤师"); }
	if (GetUpgradeRank(Upgrade) >= Row->MaxRank) { return NSLOCTEXT("Draft", "MaxRank", "已达上限"); }
	return FText::GetEmpty();
}

bool URunBuildComponent::Acquire(FName Upgrade)
{
	if (!CanAcquire(Upgrade)) { return false; }
	++Owned.FindOrAdd(Upgrade);
	OnBuildChanged.Broadcast();
	return true;
}

int32 URunBuildComponent::GetUpgradeRank(FName Id) const
{
	const int32* Rank = Owned.Find(Id);
	return Rank ? *Rank : 0;
}

bool URunBuildComponent::HasFormPermission(EPlayerTransformationForm Form) const
{
	return AvailableForms.Contains(Form);
}
