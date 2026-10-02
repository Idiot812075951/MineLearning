#include "MetaProgressComponent.h"
#include "RunContentCatalog.h"
#include "Kismet/GameplayStatics.h"

UMetaProgressComponent::UMetaProgressComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UMetaProgressComponent::Initialize(URunContentCatalog* Config)
{
	Catalog = Config;
	const bool bExists = UGameplayStatics::DoesSaveGameExist(Catalog->SaveSlot, 0);
	Profile = bExists ? Cast<UTalentProfileSaveGame>(UGameplayStatics::LoadGameFromSlot(Catalog->SaveSlot, 0)) : NewObject<UTalentProfileSaveGame>(this);
	if (!Profile || Profile->Version != 2 || Profile->TalentPoints < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[Talent] Invalid profile; original save preserved."));
		Profile = nullptr;
		return false;
	}
	return true;
}

bool UMetaProgressComponent::SaveCandidate(UTalentProfileSaveGame* Candidate)
{
	if (!Candidate || !UGameplayStatics::SaveGameToSlot(Candidate, Catalog->SaveSlot, 0))
	{
		return false;
	}
	Profile = Candidate;
	OnProfileChanged.Broadcast();
	return true;
}

bool UMetaProgressComponent::CanResearch(FName Node) const
{
	const FTalentNodeRow* Definition = Catalog ? Catalog->FindTalent(Node) : nullptr;
	if (!Profile || !Definition || Profile->Researched.Contains(Node) || Definition->Cost > Profile->TalentPoints)
	{
		return false;
	}
	for (FName Parent : Definition->Prerequisites)
	{
		if (!Profile->Researched.Contains(Parent)) { return false; }
	}
	return true;
}

bool UMetaProgressComponent::Research(FName Node)
{
	if (!CanResearch(Node))
	{
		return false;
	}
	const FTalentNodeRow* Definition = Catalog->FindTalent(Node);
	UTalentProfileSaveGame* Candidate = DuplicateObject(Profile, this);
	Candidate->TalentPoints -= Definition->Cost;
	Candidate->Researched.Add(Node);
	return SaveCandidate(Candidate);
}

bool UMetaProgressComponent::SelectSummoner(FName Id)
{
	if (!Profile || !IsSummonerUnlocked(Id)) { return false; }
	UTalentProfileSaveGame* Candidate = DuplicateObject(Profile, this);
	Candidate->SelectedSummoner = Id;
	return SaveCandidate(Candidate);
}

bool UMetaProgressComponent::AwardVictory(FGuid RunId)
{
	if (!Profile || !RunId.IsValid() || Profile->LastRewardedRun == RunId) { return false; }
	UTalentProfileSaveGame* Candidate = DuplicateObject(Profile, this);
	Candidate->TalentPoints = static_cast<int32>(FMath::Min<int64>(MAX_int32, static_cast<int64>(Candidate->TalentPoints) + Catalog->VictoryPoints));
	Candidate->LastRewardedRun = RunId;
	return SaveCandidate(Candidate);
}

bool UMetaProgressComponent::AddDebugPoints(int32 Amount)
{
	if (!Profile || Amount <= 0) { return false; }
	UTalentProfileSaveGame* Candidate = DuplicateObject(Profile, this);
	Candidate->TalentPoints = static_cast<int32>(FMath::Min<int64>(MAX_int32, static_cast<int64>(Candidate->TalentPoints) + Amount));
	return SaveCandidate(Candidate);
}

bool UMetaProgressComponent::ClearProfile()
{
	if (!Catalog) { return false; }
	if (UGameplayStatics::DoesSaveGameExist(Catalog->SaveSlot, 0) && !UGameplayStatics::DeleteGameInSlot(Catalog->SaveSlot, 0))
	{
		return false;
	}
	Profile = NewObject<UTalentProfileSaveGame>(this);
	OnProfileChanged.Broadcast();
	return true;
}

int32 UMetaProgressComponent::GetTalentPoints() const { return Profile ? Profile->TalentPoints : 0; }
bool UMetaProgressComponent::IsResearched(FName Node) const { return Profile && Profile->Researched.Contains(Node); }
FName UMetaProgressComponent::GetSelectedSummoner() const { return Profile ? Profile->SelectedSummoner : NAME_None; }

bool UMetaProgressComponent::IsSummonerUnlocked(FName Id) const
{
	if (Id.IsNone()) { return true; }
	if (!Profile || !Catalog) { return false; }
	for (FName Node : Profile->Researched)
	{
		const FTalentNodeRow* Definition = Catalog->FindTalent(Node);
		if (!Definition) { continue; }
		for (const FUnlockGrant& Grant : Definition->Grants)
		{
			if (Grant.Kind == EUnlockKind::Summoner && Grant.Id == Id) { return true; }
		}
	}
	return false;
}

const TSet<FName>& UMetaProgressComponent::GetResearchedNodes() const
{
	static const TSet<FName> Empty;
	return Profile ? Profile->Researched : Empty;
}
