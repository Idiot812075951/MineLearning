#include "MineRunCoordinatorComponent.h"
#include "MineLearning/Presentation/CombatFeedbackComponent.h"
#include "MetaProgressComponent.h"
#include "RunAbilityComponent.h"
#include "MineLearning/AI/CooperativeHaulingComponent.h"
#include "RunBuildComponent.h"
#include "UpgradeDraftComponent.h"
#include "RogueliteShop.h"
#include "MineLearning/AI/PhantomCompanionComponent.h"
#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/Combat/AmmoInventoryComponent.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/UnitEffectComponent.h"
#include "MineLearning/Combat/UnitMovementComponent.h"
#include "MineLearning/Demo/DemoRunComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/Presentation/PhantomPresentationComponent.h"
#include "MineLearning/PlayerTransformZone.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "RunCoordinator"

UMineRunCoordinatorComponent::UMineRunCoordinatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CatalogAsset = TSoftObjectPtr<URunContentCatalog>(FSoftObjectPath(TEXT("/Game/MineLearning/GameplayRuntime/Data/DA_RunContentCatalog.DA_RunContentCatalog")));
}

void UMineRunCoordinatorComponent::BeginPlay()
{
	Super::BeginPlay();
	Meta = GetOwner()->FindComponentByClass<UMetaProgressComponent>();
	Build = GetOwner()->FindComponentByClass<URunBuildComponent>();
	Draft = GetOwner()->FindComponentByClass<UUpgradeDraftComponent>();
	Phantoms = GetOwner()->FindComponentByClass<UPhantomCompanionComponent>();
	if (GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UMineRunCoordinatorComponent::Initialize);
	}
}

void UMineRunCoordinatorComponent::Initialize()
{
	Run = GetOwner()->FindComponentByClass<UDemoRunComponent>();
	if (!Run || !Run->IsEnabled()) { return; }
	Meta = GetOwner()->FindComponentByClass<UMetaProgressComponent>();
	Build = GetOwner()->FindComponentByClass<URunBuildComponent>();
	Draft = GetOwner()->FindComponentByClass<UUpgradeDraftComponent>();
	Phantoms = GetOwner()->FindComponentByClass<UPhantomCompanionComponent>();
	Catalog = CatalogAsset.LoadSynchronous();
	FString Error;
	if (!Meta || !Build || !Draft || !Phantoms || !Catalog || !Catalog->ValidateCatalog(Error) || !Meta->Initialize(Catalog))
	{
		Respond(false, FText::FromString(TEXT("初始化失败：") + Error));
		return;
	}
	bReady = true;
	if (!TActorIterator<ARogueliteShop>(GetWorld()) && !Catalog->DefaultShopClass.IsNull())
	{
		if (UClass* ShopClass = Catalog->DefaultShopClass.LoadSynchronous())
		{
			GetWorld()->SpawnActor<ARogueliteShop>(ShopClass, Catalog->DefaultShopTransform);
		}
	}
	Run->PrepareRun.BindUObject(this, &UMineRunCoordinatorComponent::PrepareRun);
	Run->HasFormPermission.BindUObject(Build, &URunBuildComponent::HasFormPermission);
	for (FName RowName : Catalog->Forms->GetRowNames())
	{
		const FFormPurchaseRow* Row = Catalog->Forms->FindRow<FFormPurchaseRow>(RowName, TEXT("Configure"));
		Run->ConfigureFormCost(Row->Form, Row->Cost);
	}
	Run->OnRunChanged.AddUniqueDynamic(this, &UMineRunCoordinatorComponent::RunChanged);
	Build->OnBuildChanged.AddUniqueDynamic(this, &UMineRunCoordinatorComponent::BuildChanged);
	Phantoms->OnCompanionChanged.AddUniqueDynamic(this, &UMineRunCoordinatorComponent::CompanionChanged);
	UnitReadyHandle = Run->OnUnitReady.AddUObject(this, &UMineRunCoordinatorComponent::AssembleUnit);
	APlayerController* Player = CastChecked<APlayerController>(GetOwner());
	Player->OnPossessedPawnChanged.AddUniqueDynamic(this, &UMineRunCoordinatorComponent::PawnChanged);
	for (TActorIterator<APlayerTransformZone> Zone(GetWorld()); Zone; ++Zone)
	{
		TransformationBindings.Add(*Zone, Zone->OnTransformationCommitted.AddUObject(this, &UMineRunCoordinatorComponent::Transformed));
	}
	AssembleUnit(Player->GetPawn());
	if (URunAbilityComponent* Abilities = GetOwner()->FindComponentByClass<URunAbilityComponent>()) { Abilities->SetControlledUnit(Player->GetPawn()); }
	OnControlledUnitReady.Broadcast(Player->GetPawn());
	Respond(true, LOCTEXT("Ready", "身份免费选择 · 专属能力局内抽取"));
}

bool UMineRunCoordinatorComponent::PrepareRun()
{
	if (!bReady || Build->IsRunActive()) { return false; }
	Build->BeginRun(Catalog, Meta->GetResearchedNodes(), Meta->GetSelectedSummoner());
	const FSummonerRow Identity = Catalog->GetSummonerData(Build->GetSummoner());
	Run->ConfigureStartingCrew(Identity.StartingOreBuddies, Identity.StartingCarriers);
	FFormPurchasedQuery Query;
	Query.BindUObject(Run, &UDemoRunComponent::IsFormUnlocked);
	Draft->Initialize(Catalog, Build, Run->GetWarehouse()->GetStorageComponent(), Query);
	return true;
}

bool UMineRunCoordinatorComponent::CanManageProfile() const
{
	return bReady && Run && (Run->GetPhase() == EDemoPhase::Briefing || Run->IsFinished());
}

bool UMineRunCoordinatorComponent::Respond(bool Success, const FText& Message)
{
	Feedback = Message;
	OnFeedbackChanged.Broadcast();
	return Success;
}

bool UMineRunCoordinatorComponent::Research(FName Node)
{
	const bool Success = CanManageProfile() && Meta->Research(Node);
	return Respond(Success, Success ? LOCTEXT("Researched", "已解锁并保存") : LOCTEXT("ResearchFailed", "点数、前置或阶段不满足"));
}

bool UMineRunCoordinatorComponent::SelectSummoner(FName Id)
{
	const bool Success = CanManageProfile() && Meta->SelectSummoner(Id);
	return Respond(Success, Success ? LOCTEXT("IdentitySelected", "身份已选择") : LOCTEXT("IdentityFailed", "身份未解锁或本局已锁定"));
}

bool UMineRunCoordinatorComponent::BuyDraft()
{
	const bool Success = CanPurchaseDraft() && Draft->BuyOffer();
	return Respond(Success, Success ? LOCTEXT("Paid", "选择一张升级卡") : LOCTEXT("PurchaseFailed", "请靠近商店，并检查金币和候选资格"));
}

bool UMineRunCoordinatorComponent::ChooseUpgrade(int32 OfferId, FName Id)
{
	const bool Success = bReady && Draft->Choose(OfferId, Id);
	return Respond(Success, Success ? LOCTEXT("Acquired", "本局获得升级。") : LOCTEXT("ChoiceFailed", "选择无效：候选已消费、局已结束或资格不足。"));
}

void UMineRunCoordinatorComponent::AssembleUnit(APawn* Unit)
{
	if (!bReady || !IsValid(Unit) || !Unit->HasAuthority() || !Unit->FindComponentByClass<UCombatComponent>()) { return; }
	Units.RemoveAll([](const TWeakObjectPtr<APawn>& Entry) { return !Entry.IsValid(); });
	Units.AddUnique(Unit);
	if (UResourceCarryComponent* Carry = Unit->FindComponentByClass<UResourceCarryComponent>())
	{
		Carry->OnPickupCompleted.AddUniqueDynamic(this, &UMineRunCoordinatorComponent::PickupCompleted);
	}
	if (!Unit->FindComponentByClass<UUnitMovementComponent>())
	{
		UUnitMovementComponent* Movement = NewObject<UUnitMovementComponent>(Unit);
		Unit->AddInstanceComponent(Movement);
		Movement->RegisterComponent();
	}
	UUnitEffectComponent* Effects = Unit->FindComponentByClass<UUnitEffectComponent>();
	if (!Effects)
	{
		Effects = NewObject<UUnitEffectComponent>(Unit);
		Unit->AddInstanceComponent(Effects);
		Effects->RegisterComponent();
	}
	if (!Unit->FindComponentByClass<UCombatFeedbackComponent>())
	{
		UCombatFeedbackComponent* CombatFeedback = NewObject<UCombatFeedbackComponent>(Unit);
		Unit->AddInstanceComponent(CombatFeedback);
		CombatFeedback->RegisterComponent();
	}
	APlayerController* Player = CastChecked<APlayerController>(GetOwner());
	if (AGunnerCharacter* Gunner = Cast<AGunnerCharacter>(Unit); Gunner && Player->GetPawn() == Unit)
	{
		Gunner->SetAmmoAccount(GetOwner()->FindComponentByClass<UAmmoInventoryComponent>());
	}
	TSet<FName> Desired;
	for (const TPair<FName, int32>& Owned : Build->GetOwnedUpgrades())
	{
		if (!Build->IsRunActive())
		{
			break;
		}
		const FUpgradeRow* Upgrade = Catalog->FindUpgrade(Owned.Key);
		if (!Upgrade || (Upgrade->Audience == EUpgradeAudience::ControlledPlayer && Player->GetPawn() != Unit)
			|| (!Upgrade->bIncludePhantoms && Unit->ActorHasTag(TEXT("Phantom")))) { continue; }
		if (!Upgrade->UnitClasses.IsEmpty() && !Upgrade->UnitClasses.ContainsByPredicate([Unit](const TSoftClassPtr<APawn>& Class)
			{ const UClass* UnitClass = Class.LoadSynchronous(); return UnitClass && Unit->IsA(UnitClass); })) { continue; }
		for (int32 Rank = 0; Rank < Owned.Value; ++Rank)
		{
			for (UUnitEffectDefinition* Definition : Upgrade->Effects)
			{
				const FName Source(*FString::Printf(TEXT("Upgrade:%s:%d:%s"), *Owned.Key.ToString(), Rank, *Definition->Rule.Id.ToString()));
				Desired.Add(Source);
				if (!Effects->GrantDefinition(Source, Definition))
				{
					UE_LOG(LogTemp, Error, TEXT("[Run] Cannot grant %s to %s"), *Source.ToString(), *Unit->GetName());
				}
			}
		}
	}
	for (FName Source : Effects->GetGrantedSources())
	{
		if (Source.ToString().StartsWith(TEXT("Upgrade:")) && !Desired.Contains(Source))
		{
			Effects->RevokeDefinition(Source);
		}
	}
	if (URunAbilityComponent* Abilities = GetOwner()->FindComponentByClass<URunAbilityComponent>()) { Abilities->RegisterUnit(Unit); }
	if (UCooperativeHaulingComponent* Hauling = GetOwner()->FindComponentByClass<UCooperativeHaulingComponent>()) { Hauling->RegisterUnit(Unit); }
}

void UMineRunCoordinatorComponent::RefreshUnits()
{
	const TArray<TWeakObjectPtr<APawn>> Snapshot = Units;
	for (const TWeakObjectPtr<APawn>& Unit : Snapshot) { AssembleUnit(Unit.Get()); }
}

void UMineRunCoordinatorComponent::BuildChanged()
{
	if (!bReady) { return; }
	RefreshUnits();
	bool Enabled = false;
	UOverclockDefinition* Overclock = nullptr;
	UAIWorkDefinition* Work = nullptr;
	URelayHaulingDefinition* Relay = nullptr;
	USharedCarryDefinition* Pair = nullptr;
	for (const TPair<FName, int32>& Owned : Build->GetOwnedUpgrades())
	{
		const FUpgradeRow* Row = Catalog->FindUpgrade(Owned.Key);
		Enabled |= Build->IsRunActive() && Row && Row->bGrantsPhantomCompanion;
		if (Build->IsRunActive() && Row)
		{
			if (Row->OverclockAbility) { Overclock = Row->OverclockAbility; }
			if (Row->AIWorkAbility) { Work = Row->AIWorkAbility; }
			if (Row->RelayAbility) { Relay = Row->RelayAbility; }
			if (Row->SharedCarryAbility) { Pair = Row->SharedCarryAbility; }
		}
	}
	if (URunAbilityComponent* Abilities = GetOwner()->FindComponentByClass<URunAbilityComponent>()) { Abilities->Configure(Overclock, Work); }
	if (UCooperativeHaulingComponent* Hauling = GetOwner()->FindComponentByClass<UCooperativeHaulingComponent>()) { Hauling->Configure(Relay, Pair); }
	if (Run && Run->GetWarehouse()) { Run->GetWarehouse()->SetDispatchBatchSize(Pair ? Pair->DispatchBatchSize : 4); }
	if (Enabled && !bPhantomAbilityActive)
	{
		bPhantomAbilityActive = true;
		Phantoms->SpawnFor(CastChecked<APlayerController>(GetOwner())->GetPawn(), Catalog->PhantomLifetime);
	}
	else if (!Enabled && bPhantomAbilityActive)
	{
		bPhantomAbilityActive = false;
		Phantoms->Clear();
	}
}

void UMineRunCoordinatorComponent::PawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (!bReady) { return; }
	if (UWeaponActionComponent* Weapon = OldPawn ? OldPawn->FindComponentByClass<UWeaponActionComponent>() : nullptr)
	{
		SavedPlayerWeapon = Weapon->Export();
		bHasSavedPlayerWeapon = true;
	}
	if (UWeaponActionComponent* Weapon = NewPawn ? NewPawn->FindComponentByClass<UWeaponActionComponent>() : nullptr)
	{
		if (bHasSavedPlayerWeapon)
		{
			Weapon->Restore(SavedPlayerWeapon);
			bHasSavedPlayerWeapon = false;
		}
		else
		{
			FWeaponRuntimeState Initial = Weapon->Export();
			Initial.Ammo = 0; // A purchased form does not include a free magazine.
			Weapon->Restore(Initial);
		}
	}
	AssembleUnit(OldPawn);
	AssembleUnit(NewPawn);
	if (URunAbilityComponent* Abilities = GetOwner()->FindComponentByClass<URunAbilityComponent>()) { Abilities->SetControlledUnit(NewPawn); }
	if (UUnitEffectComponent* Effects = NewPawn ? NewPawn->FindComponentByClass<UUnitEffectComponent>() : nullptr) { Effects->RefreshRuntimeState(); }
	if (!NewPawn) { Phantoms->Clear(); }
	OnControlledUnitReady.Broadcast(NewPawn);
}

void UMineRunCoordinatorComponent::Transformed(APlayerController* Player, APawn* Unit, EPlayerTransformationForm Form)
{
	if (Player == GetOwner())
	{
		if (AGunnerCharacter* Gunner = Cast<AGunnerCharacter>(Unit))
		{
			Gunner->TryLoadEmptyMagazine();
		}
	}
	if (Player == GetOwner() && Build->IsRunActive() && bPhantomAbilityActive)
	{
		Phantoms->SpawnFor(Unit, Catalog->PhantomLifetime);
	}
}

void UMineRunCoordinatorComponent::CompanionChanged()
{
	APawn* Unit = Phantoms->GetCompanion();
	if (!Unit) { return; }
	AssembleUnit(Unit);
	if (UUnitEffectComponent* Effects = Unit->FindComponentByClass<UUnitEffectComponent>())
	{
		Effects->GrantDefinition(TEXT("Phantom:Haste"), Catalog->PhantomHaste);
	}
	UPhantomPresentationComponent* Presentation = NewObject<UPhantomPresentationComponent>(Unit);
	Presentation->Material = Catalog->PhantomMaterial;
	Unit->AddInstanceComponent(Presentation);
	Presentation->RegisterComponent();
}

void UMineRunCoordinatorComponent::PickupCompleted(AActor* Collector, EItemType Item, int32 Amount)
{
	if (IsValid(Collector) && Amount > 0)
	{
		if (UUnitEffectComponent* Effects = Collector->FindComponentByClass<UUnitEffectComponent>())
		{
			Effects->HandleEvent(TEXT("PickupCompleted"));
		}
	}
}

void UMineRunCoordinatorComponent::RunChanged()
{
	if (!bReady || bHandlingRun) { return; }
	TGuardValue<bool> Guard(bHandlingRun, true);
	if (Run->IsFinished() && Build->GetRunId().IsValid())
	{
		Meta->AwardVictory(Build->GetRunId());
		if (Build->IsRunActive())
		{
			Build->EndRun();
			Draft->ResetOffer();
			Phantoms->Clear();
		}
	}
}

void UMineRunCoordinatorComponent::GrantDebugPoints(int32 Amount)
{
	if (bReady) { Respond(Meta->AddDebugPoints(Amount), LOCTEXT("GMPoints", "GM 天赋点操作已执行；可在局间研究。")); }
}

void UMineRunCoordinatorComponent::GrantDebugResources()
{
	if (bReady && Run->GetWarehouse()) { Run->GetWarehouse()->GetStorageComponent()->GrantDebugStock(10000); }
}

bool UMineRunCoordinatorComponent::GrantDebugUpgrade(FName Id)
{
	return bReady && Build->Acquire(Id);
}

void UMineRunCoordinatorComponent::SetReloadChance(float Chance)
{
	for (const TWeakObjectPtr<APawn>& Unit : Units)
	{
		if (Unit.IsValid())
		{
			if (UWeaponActionComponent* Weapon = Unit->FindComponentByClass<UWeaponActionComponent>())
			{
				Weapon->ReloadChanceOverride = FMath::Clamp(Chance, -1.f, 1.f);
			}
		}
	}
}

bool UMineRunCoordinatorComponent::ClearProfileAndRestart()
{
	if (!Catalog || !Meta || !Build || !Draft || !Phantoms || !Meta->ClearProfile()) { return false; }
	Phantoms->Clear();
	Draft->ResetOffer();
	Build->EndRun();
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
	return true;
}

void UMineRunCoordinatorComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	for (const TWeakObjectPtr<APawn>& Unit : Units)
	{
		if (Unit.IsValid())
		{
			if (UResourceCarryComponent* Carry = Unit->FindComponentByClass<UResourceCarryComponent>()) { Carry->OnPickupCompleted.RemoveDynamic(this, &UMineRunCoordinatorComponent::PickupCompleted); }
		}
	}
	for (const TPair<TWeakObjectPtr<APlayerTransformZone>, FDelegateHandle>& Binding : TransformationBindings)
	{
		if (Binding.Key.IsValid()) { Binding.Key->OnTransformationCommitted.Remove(Binding.Value); }
	}
	if (Run)
	{
		Run->PrepareRun.Unbind();
		Run->HasFormPermission.Unbind();
		Run->OnRunChanged.RemoveDynamic(this, &UMineRunCoordinatorComponent::RunChanged);
		Run->OnUnitReady.Remove(UnitReadyHandle);
	}
	if (Build) { Build->OnBuildChanged.RemoveDynamic(this, &UMineRunCoordinatorComponent::BuildChanged); }
	if (Phantoms) { Phantoms->OnCompanionChanged.RemoveDynamic(this, &UMineRunCoordinatorComponent::CompanionChanged); }
	if (APlayerController* Player = Cast<APlayerController>(GetOwner())) { Player->OnPossessedPawnChanged.RemoveDynamic(this, &UMineRunCoordinatorComponent::PawnChanged); }
	Super::EndPlay(Reason);
}

#undef LOCTEXT_NAMESPACE
