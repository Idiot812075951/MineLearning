#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/UnitEffectComponent.h"
#include "MineLearning/Combat/UnitEffectDefinition.h"
#include "MineLearning/Combat/WeaponActionComponent.h"
#include "MineLearning/Roguelite/RunContentCatalog.h"
#include "MineLearning/Roguelite/RunBuildComponent.h"
#include "MineLearning/Roguelite/UpgradeDraftComponent.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/Roguelite/MetaProgressComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunCatalogTest, "MineLearning.Roguelite.Catalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRunCatalogTest::RunTest(const FString& Parameters)
{
	URunContentCatalog* Catalog = LoadObject<URunContentCatalog>(nullptr,
		TEXT("/Game/MineLearning/GameplayRuntime/Data/DA_RunContentCatalog.DA_RunContentCatalog"));
	if (!TestNotNull(TEXT("Authored UE catalog"), Catalog)) { return false; }
	FString Error;
	const bool Valid = Catalog->ValidateCatalog(Error);
	TestTrue(*Error, Valid);
	TestTrue(TEXT("Paid draft cost"), !Catalog->DraftCost.IsEmpty() && Catalog->DraftCost[0].Amount > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponEffectsTest, "MineLearning.Roguelite.WeaponEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponEffectsTest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	AActor* Unit = World->SpawnActor<AActor>();
	UCombatComponent* Combat = NewObject<UCombatComponent>(Unit);
	Unit->AddInstanceComponent(Combat);
	Combat->RegisterComponent();
	UWeaponActionComponent* Weapon = NewObject<UWeaponActionComponent>(Unit);
	Unit->AddInstanceComponent(Weapon);
	Weapon->RegisterComponent();
	Weapon->Initialize(100);
	UUnitEffectComponent* Effects = NewObject<UUnitEffectComponent>(Unit);
	Unit->AddInstanceComponent(Effects);
	Effects->RegisterComponent();
	Unit->DispatchBeginPlay();
	UShotSequenceEffectDefinition* Sequence = NewObject<UShotSequenceEffectDefinition>();
	Sequence->Rule.Id = TEXT("Sequence");
	TestTrue(TEXT("Grant behavior"), Effects->GrantDefinition(TEXT("Test:Sequence"), Sequence));
	for (int32 Index = 1; Index <= 30; ++Index)
	{
		const FWeaponShotContext Context = Weapon->PrepareShot();
		TestEqual(TEXT("Tenth emissions only"), Context.DamageMultiplier, Index % 10 == 0 ? 2.f : 1.f);
		Weapon->ConsumeRound();
		Weapon->CommitShot();
	}
	TestEqual(TEXT("Per-emission ammo"), Weapon->GetCurrentAmmo(), 70);
	UReloadCapacityEffectDefinition* Reload = NewObject<UReloadCapacityEffectDefinition>();
	Reload->Rule.Id = TEXT("Reload");
	Effects->GrantDefinition(TEXT("Test:Reload"), Reload);
	Weapon->ReloadChanceOverride = 1.f;
	Weapon->CommitReload(20);
	TestEqual(TEXT("Double capacity"), Weapon->GetCapacity(), 40);
	Weapon->CommitReload(20);
	TestEqual(TEXT("No capacity escalation"), Weapon->GetCapacity(), 40);
	Weapon->ReloadChanceOverride = 0.f;
	Weapon->CommitReload(20);
	TestEqual(TEXT("Normal capacity"), Weapon->GetCapacity(), 20);
	Effects->RevokeDefinition(TEXT("Test:Sequence"));
	TestEqual(TEXT("Revoke hook"), Weapon->PrepareShot().DamageMultiplier, 1.f);
	Unit->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDraftPurchaseTest, "MineLearning.Roguelite.PaidDraft",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDraftPurchaseTest::RunTest(const FString& Parameters)
{
	URunContentCatalog* Catalog = LoadObject<URunContentCatalog>(nullptr,
		TEXT("/Game/MineLearning/GameplayRuntime/Data/DA_RunContentCatalog.DA_RunContentCatalog"));
	if (!Catalog) { return false; }
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	AActor* Owner = World->SpawnActor<AActor>();
	URunBuildComponent* Build = NewObject<URunBuildComponent>(Owner);
	UUpgradeDraftComponent* Draft = NewObject<UUpgradeDraftComponent>(Owner);
	UResourceStorageComponent* Wallet = NewObject<UResourceStorageComponent>(Owner);
	Owner->AddInstanceComponent(Wallet);
	Wallet->RegisterComponent();
	Build->BeginRun(Catalog, {}, NAME_None);
	FFormPurchasedQuery Query;
	Query.BindLambda([](EPlayerTransformationForm) { return true; });
	Draft->Initialize(Catalog, Build, Wallet, Query);
	TestFalse(TEXT("No money"), Draft->BuyOffer());
	Wallet->GrantDebugStock(1000);
	const EItemType Currency = Catalog->DraftCost[0].ItemType;
	const int32 Before = Wallet->GetAvailableItemAmount(Currency);
	TestTrue(TEXT("Paid purchase"), Draft->BuyOffer());
	const TArray<FName> Offer = Draft->GetCandidates();
	TSet<FName> Unique;
	for (FName Id : Offer) { Unique.Add(Id); }
	TestTrue(TEXT("Up to three unique candidates"), Offer.Num() > 0 && Offer.Num() <= 3 && Unique.Num() == Offer.Num());
	const int32 PaidBalance = Wallet->GetAvailableItemAmount(Currency);
	TestTrue(TEXT("Charged"), PaidBalance < Before);
	TestEqual(TEXT("First purchase price"), PaidBalance, Before - 4);
	TestEqual(TEXT("Next price is eight"), Draft->GetNextCost()[0].Amount, 8);
	TestFalse(TEXT("Duplicate purchase"), Draft->BuyOffer());
	TestEqual(TEXT("No duplicate fee"), Wallet->GetAvailableItemAmount(Currency), PaidBalance);
	if (!Offer.IsEmpty())
	{
		const int32 Id = Draft->GetOfferId();
		TestTrue(TEXT("Choose current offer"), Draft->Choose(Id, Offer[0]));
		TestFalse(TEXT("Double choice"), Draft->Choose(Id, Offer[0]));
	}
	TestTrue(TEXT("Second purchase"), Draft->BuyOffer());
	TestEqual(TEXT("Second price charged"), Wallet->GetAvailableItemAmount(Currency), PaidBalance - 8);
	TestEqual(TEXT("Third price is twelve"), Draft->GetNextCost()[0].Amount, 12);
	Draft->Choose(Draft->GetOfferId(), Draft->GetCandidates()[0]);
	TestTrue(TEXT("Third purchase"), Draft->BuyOffer());
	TestEqual(TEXT("Third price charged"), Wallet->GetAvailableItemAmount(Currency), PaidBalance - 20);
	Draft->ResetOffer();
	TestEqual(TEXT("Discarding offer does not reset price"), Draft->GetNextCost()[0].Amount, 16);
	Draft->Initialize(Catalog, Build, Wallet, Query);
	TestEqual(TEXT("New run resets price"), Draft->GetNextCost()[0].Amount, 4);
	TestTrue(TEXT("Wrong identity is excluded"), !Draft->GetEligibilityBlock(TEXT("Phantom")).IsEmpty());
	TestTrue(TEXT("Talent lock is excluded"), !Draft->GetEligibilityBlock(TEXT("SuperRound")).IsEmpty());
	Owner->Destroy();
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTalentPersistenceTest, "MineLearning.Roguelite.TalentPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTalentPersistenceTest::RunTest(const FString& Parameters)
{
	URunContentCatalog* Original = LoadObject<URunContentCatalog>(nullptr,
		TEXT("/Game/MineLearning/GameplayRuntime/Data/DA_RunContentCatalog.DA_RunContentCatalog"));
	if (!Original)
	{
		return false;
	}
	URunContentCatalog* Catalog = DuplicateObject(Original, GetTransientPackage());
	Catalog->SaveSlot = TEXT("Automation_Talent_") + FGuid::NewGuid().ToString();
	UMetaProgressComponent* Meta = NewObject<UMetaProgressComponent>();
	TestTrue(TEXT("Fresh profile"), Meta->Initialize(Catalog));
	TestFalse(TEXT("No free talent"), Meta->Research(TEXT("SuperRound")));
	TestTrue(TEXT("Grant points"), Meta->AddDebugPoints(20));
	TestFalse(TEXT("Prerequisite enforced"), Meta->Research(TEXT("SuperMagazine")));
	TestTrue(TEXT("Research predecessor"), Meta->Research(TEXT("SuperRound")));
	TestTrue(TEXT("Research successor"), Meta->Research(TEXT("SuperMagazine")));
	TestFalse(TEXT("No repeat point spend"), Meta->Research(TEXT("SuperRound")));
	URunBuildComponent* Build = NewObject<URunBuildComponent>();
	Build->BeginRun(Catalog, Meta->GetResearchedNodes(), NAME_None);
	TestTrue(TEXT("Unlocked eligibility"), Build->CanAcquire(TEXT("SuperRound")));
	TestEqual(TEXT("Research is not ownership"), Build->GetUpgradeRank(TEXT("SuperRound")), 0);
	const FGuid RunId = Build->GetRunId();
	const int32 Before = Meta->GetTalentPoints();
	TestTrue(TEXT("Victory reward"), Meta->AwardVictory(RunId));
	TestFalse(TEXT("Reward once per run"), Meta->AwardVictory(RunId));
	TestEqual(TEXT("Reward points"), Meta->GetTalentPoints(), Before + Catalog->VictoryPoints);
	UMetaProgressComponent* Reloaded = NewObject<UMetaProgressComponent>();
	TestTrue(TEXT("Reload profile"), Reloaded->Initialize(Catalog));
	TestTrue(TEXT("Persistent researched talent"), Reloaded->IsResearched(TEXT("SuperMagazine")));
	TestEqual(TEXT("Persistent balance"), Reloaded->GetTalentPoints(), Meta->GetTalentPoints());
	TestTrue(TEXT("Clear isolated test save"), Meta->ClearProfile());
	TestTrue(TEXT("Frozen run eligibility survives profile change"), Build->CanAcquire(TEXT("SuperRound")));
	TestFalse(TEXT("Deleted isolated save"), UGameplayStatics::DoesSaveGameExist(Catalog->SaveSlot, 0));
	return true;
}
#endif
