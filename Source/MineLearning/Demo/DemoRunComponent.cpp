#include "DemoRunComponent.h"

#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/AI/HaulerCharacter.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "MineLearning/Combat/AmmoInventoryComponent.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/MiningGameSubsystem.h"
#include "MineLearning/Mining/MiningPlayerData.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "DemoRun"

UDemoRunComponent::UDemoRunComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CarrierClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/MineLearning/Mining/Logistics/Blueprints/BP_Hauler.BP_Hauler_C")));
	OreBuddyClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/MineLearning/Characters/OreBuddy/Blueprints/BP_OreBuddy07.BP_OreBuddy07_C")));
	BossClass = TSoftClassPtr<AMineableOre>(FSoftObjectPath(TEXT("/Game/MineLearning/Mining/Ores/Iron/Blueprints/BP_SuperIronOre.BP_SuperIronOre_C")));
}

bool UDemoRunComponent::IsEnabled() const
{
	return GetWorld() && GetWorld()->GetMapName().Contains(TEXT("L_WorldLayout_P01"));
}

void UDemoRunComponent::BeginPlay()
{
	Super::BeginPlay();
	if (IsEnabled() && GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UDemoRunComponent::InitializeRun);
	}
}

void UDemoRunComponent::InitializeRun()
{
	for (TActorIterator<AWarehouseDepot> It(GetWorld()); It; ++It)
	{
		Warehouse = *It;
		break;
	}
	if (!Warehouse)
	{
		Respond(false, LOCTEXT("MissingWarehouse", "矿区仓库未加载，无法开始试运行。"));
		return;
	}
	UResourceStorageComponent* Storage = Warehouse->GetStorageComponent();
	for (EItemType Type : {EItemType::IronOre, EItemType::IronIngot, EItemType::Coin})
	{
		Warehouse->CancelPendingOrder(Type, MAX_int32);
		Storage->RemoveItem({Type, Storage->GetAvailableItemAmount(Type)});
	}
	Storage->OnInventoryChanged.AddUniqueDynamic(this, &UDemoRunComponent::InventoryChanged);
	// Editor demonstration workers must not silently provide a free production line.
	TArray<APawn*> PlacedWorkers;
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		if (!It->IsPlayerControlled() && (It->IsA<AHaulerCharacter>() || It->IsA<AMiningCompanionCharacter>() || It->IsA<AGunnerCharacter>()))
		{
			PlacedWorkers.Add(*It);
		}
	}
	for (APawn* Worker : PlacedWorkers)
	{
		if (AController* Controller = Worker->GetController())
		{
			Controller->Destroy();
		}
		Worker->Destroy();
	}
	UMiningPlayerData* Data = GetWorld()->GetGameInstance()->GetSubsystem<UMiningGameSubsystem>()->GetPlayerData();
	Data->ReleasePopulation(Data->PopulationUsed);
	Data->ConsumeProcessedOre(Data->ProcessedOre);
	Respond(true, LOCTEXT("WelcomeBoss", "选择召唤师，然后开始本局"));
}

void UDemoRunComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	if (Warehouse)
	{
		Warehouse->GetStorageComponent()->OnInventoryChanged.RemoveDynamic(this, &UDemoRunComponent::InventoryChanged);
	}
	if (IsValid(BossTarget))
	{
		BossTarget->FindComponentByClass<UHealthComponent>()->OnDamageResolved.RemoveDynamic(this, &UDemoRunComponent::TargetDamaged);
		BossTarget->OnDestroyed.RemoveDynamic(this, &UDemoRunComponent::BossDestroyed);
	}
	for (APawn* Worker : Workers)
	{
		if (IsValid(Worker))
		{
			Worker->OnDestroyed.RemoveDynamic(this, &UDemoRunComponent::WorkerDestroyed);
		}
	}
	Super::EndPlay(Reason);
}

bool UDemoRunComponent::IsFinished() const
{
	return Phase == EDemoPhase::Victory;
}

void UDemoRunComponent::AdvanceClock()
{
	if (Phase != EDemoPhase::BossChallenge) { return; }
	RemainingSeconds = FMath::Max(0.f, ChallengeDuration - (GetWorld()->GetTimeSeconds() - BossStartedAt));
	if (RemainingSeconds <= 0.f)
	{
		EndBossAttempt();
		return;
	}
	OnRunChanged.Broadcast();
}

void UDemoRunComponent::Finish()
{
	if (IsFinished())
	{
		return;
	}
	Phase = EDemoPhase::Victory;
	FinishedAt = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().ClearTimer(ClockHandle);
	Feedback = LOCTEXT("BossDefeated", "超级铁矿已击破！矿区挑战胜利。可继续生产、切换形态，或从终端重新开始。" );
	UE_LOG(LogTemp, Display, TEXT("[DemoRun] Boss Victory Duration=%.1f Attempts=%d"), FinishedAt - StartedAt, BossAttempts);
	OnRunChanged.Broadcast();
}

bool UDemoRunComponent::Respond(bool bSuccess, const FText& Message)
{
	Feedback = Message;
	OnRunChanged.Broadcast();
	return bSuccess;
}

int32 UDemoRunComponent::Available(EItemType Item) const
{
	return Warehouse ? Warehouse->GetStorageComponent()->GetAvailableItemAmount(Item) : 0;
}

bool UDemoRunComponent::ExecuteCommand(EDemoCommand Command)
{
	if (!IsEnabled() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	if (Command == EDemoCommand::Restart)
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
		return true;
	}
	if (Command == EDemoCommand::Start && Phase == EDemoPhase::Briefing && Warehouse)
	{
		if (!PrepareRun.IsBound() || !PrepareRun.Execute()) { return Respond(false, LOCTEXT("RunNotReady", "玩法配置或天赋存档未就绪，未开工。")); }
		Phase = EDemoPhase::Production;
		StartedAt = GetWorld()->GetTimeSeconds();
		ChangeForm(EPlayerTransformationForm::OreBuddy);
		return Respond(true, LOCTEXT("StartedFree", "Q 钻采 / R 拾取 · E 交互 · Tab 生产 · 商店抽升级"));
	}
	if (Command == EDemoCommand::UnlockAll && Warehouse)
	{
		if (Phase == EDemoPhase::Briefing)
		{
			if (!PrepareRun.IsBound() || !PrepareRun.Execute()) { return false; }
			Phase = EDemoPhase::Production;
			StartedAt = GetWorld()->GetTimeSeconds();
		}
		bGunnerUnlocked = true;
		bGurenUnlocked = true;
		CoreBonus.Strength = 15.f;
		CoreBonus.Agility = 15.f;
		CoreBonus.Intelligence = 15.f;
		if (UAmmoInventoryComponent* Account = GetOwner()->FindComponentByClass<UAmmoInventoryComponent>()) { Account->AddMagazines(FMath::Max(0, 999 - Account->GetAvailableMagazines())); }
		APlayerController* PC = Cast<APlayerController>(GetOwner());
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		ApplyCoreBonus(Pawn);
		for (APawn* Worker : Workers)
		{
			ApplyCoreBonus(Worker);
		}
		if (AGunnerCharacter* Gunner = Cast<AGunnerCharacter>(Pawn))
		{
			// Do not interrupt an in-flight burst or paid reload.
			if (!Gunner->IsWeaponBusy())
			{
				Gunner->RestoreLoadedAmmo(Gunner->GetMagazineSize());
			}
		}

		Warehouse->GetStorageComponent()->GrantDebugStock(10000);
		return Respond(true, LOCTEXT("GMUnlockAll", "GM：全部形态与功能已解锁，核心升级满级；各类资源补足 10000，备用弹匣补足 999。"));
	}
	if (Phase == EDemoPhase::Briefing || !Warehouse)
	{
		return Respond(false, LOCTEXT("NotRunning", "请先开始试运行；结算后可重开一局。"));
	}
	AdvanceClock();
	UResourceStorageComponent* Storage = Warehouse->GetStorageComponent();
	switch (Command)
	{
	case EDemoCommand::ProcessFour:
	{
		const int32 Amount = FMath::Min(4, Storage->GetAvailableOre());
		const bool bOK = Amount >= 2 && Warehouse->RequestProcess(EItemType::IronOre, Amount - Amount % 2);
		return Respond(bOK, bOK ? LOCTEXT("ProcessOK", "加工订单已预留。Carrier 在仓库入口按鼠标左键 装货，送到加工机入口；自动 Carrier 也会接单。") : LOCTEXT("ProcessNo", "需要至少 2 块可用原矿；已预留的资源不能重复下单。"));
	}
	case EDemoCommand::SellTwo:
	{
		const int32 Amount = FMath::Min(2, Storage->GetAvailableItemAmount(EItemType::IronIngot));
		const bool bOK = Amount > 0 && Warehouse->RequestSell(EItemType::IronIngot, Amount);
		return Respond(bOK, bOK ? LOCTEXT("SellOK", "销售订单已预留。送达出售点后，每块铁锭产出 2 金币；金币需运回仓库才能使用。") : LOCTEXT("SellNo", "仓库没有可出售的铁锭，或出售点正忙；稍后再试。"));
	}
	case EDemoCommand::CancelOrders:
		Warehouse->CancelPendingOrder(EItemType::IronOre, MAX_int32);
		Warehouse->CancelPendingOrder(EItemType::IronIngot, MAX_int32);
		return Respond(true, LOCTEXT("Cancel", "尚未提货的订单已取消，库存解冻；运输中的货物继续送达。"));
	case EDemoCommand::BuyCarrier: return PurchaseRobot(true);
	case EDemoCommand::BuyOreBuddy: return PurchaseRobot(false);
	case EDemoCommand::UpgradeStrength:
	case EDemoCommand::UpgradeAgility:
	case EDemoCommand::UpgradeIntelligence:
	{
		float& Attribute = Command == EDemoCommand::UpgradeStrength ? CoreBonus.Strength
			: (Command == EDemoCommand::UpgradeAgility ? CoreBonus.Agility : CoreBonus.Intelligence);
		const int32 Level = FMath::RoundToInt(Attribute / 5.f);
		if (Level >= 3) { return Respond(false, LOCTEXT("UpgradeCap", "该项核心升级已达 3 级（+15）。升级是可选强化，不是通关条件。")); }
		const int32 Price = 2 * (Level + 1);
		if (!Storage->RemoveItem({EItemType::Coin, Price}))
		{
			return Respond(false, FText::Format(LOCTEXT("UpgradePrice", "本级核心升级需要 {0} 金币，每项最多 3 级。"), FText::AsNumber(Price)));
		}
		Attribute += 5.f;
		ApplyCoreBonus(Cast<APlayerController>(GetOwner())->GetPawn());
		for (APawn* Worker : Workers) { ApplyCoreBonus(Worker); }
		InventoryChanged();
		return Respond(true, LOCTEXT("UpgradeOK", "核心升级完成：对应属性 +5，立即应用于玩家所有形态和本局自动机器人。"));
	}
	case EDemoCommand::UnlockGunner:
	case EDemoCommand::UnlockGuren:
	{
		bool& bUnlocked = Command == EDemoCommand::UnlockGunner ? bGunnerUnlocked : bGurenUnlocked;
		if (bUnlocked) { return Respond(false, LOCTEXT("AlreadyUnlocked", "该形态本局已经解锁。")); }
		const EPlayerTransformationForm Form = Command == EDemoCommand::UnlockGunner ? EPlayerTransformationForm::Gunner : EPlayerTransformationForm::Guren;
		const TArray<FItemStack>* Cost = FormCosts.Find(Form);
		if (!HasFormPermission.IsBound() || !HasFormPermission.Execute(Form))
		{
			return Respond(false, LOCTEXT("TalentRequired", "本局没有该形态的天赋许可；研究后下一局生效。"));
		}
		if (!Cost || !Storage->TrySpendItems(*Cost))
		{
			return Respond(false, LOCTEXT("FormCost", "仓库可用资源不足，或形态购买配置缺失。"));
		}
		bUnlocked = true;
		InventoryChanged();
		return Respond(true, LOCTEXT("UnlockOK", "形态已解锁，可在现场终端免费切换，不占自动机器人名额。"));
	}
	case EDemoCommand::BuyMagazine:
	{
		if (!bGunnerUnlocked) { return Respond(false, LOCTEXT("AmmoLocked", "先解锁 Gunner，再购买弹匣。")); }
		if (!Storage->RemoveItem({EItemType::IronIngot, 1})) { return Respond(false, LOCTEXT("AmmoCost", "1 块仓库可用铁锭可购买 1 个弹匣（20 发）。")); }
		GetOwner()->FindComponentByClass<UAmmoInventoryComponent>()->AddMagazines(1);
		InventoryChanged();
		return Respond(true, LOCTEXT("AmmoBought", "备用弹匣 +1（20 发）。Gunner 按 R 装填；换形态保留子弹与备用弹匣。"));
	}
	case EDemoCommand::SubmitMaterials: return BeginBossChallenge();
	case EDemoCommand::Human: return ChangeForm(EPlayerTransformationForm::Human);
	case EDemoCommand::OreBuddy: return ChangeForm(EPlayerTransformationForm::OreBuddy);
	case EDemoCommand::Carrier: return ChangeForm(EPlayerTransformationForm::Carrier);
	case EDemoCommand::Gunner: return ChangeForm(EPlayerTransformationForm::Gunner);
	case EDemoCommand::Guren: return ChangeForm(EPlayerTransformationForm::Guren);
	case EDemoCommand::ResetCalibration:
		return BeginBossChallenge();
	default: return false;
	}
}

bool UDemoRunComponent::PurchaseRobot(bool bCarrier)
{
	Workers.RemoveAll([](const APawn* Pawn) { return !IsValid(Pawn); });
	if (Workers.Num() >= 3)
	{
		return Respond(false, LOCTEXT("PopulationFull", "自动机器人名额已满（3 台）；玩家幻化不占名额。"));
	}
	const int32 Price = bCarrier ? 4 : 8;
	if (Available(EItemType::Coin) < Price)
	{
		return Respond(false, LOCTEXT("RobotPrice", "可用金币不足：自动 Carrier 4 金币，自动 OreBuddy 8 金币。"));
	}
	UClass* Class = bCarrier ? CarrierClass.LoadSynchronous() : OreBuddyClass.LoadSynchronous();
	FVector SpawnLocation(100.f, 1750.f, 350.f);
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<USceneComponent*> Components(*It);
		for (USceneComponent* Component : Components)
		{
			if (Component->GetFName() == TEXT("RobotJoinNavPoint"))
			{
				SpawnLocation = Component->GetComponentLocation() + FVector(0.f, 0.f, 100.f);
			}
		}
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	APawn* Robot = Class ? GetWorld()->SpawnActor<APawn>(Class, SpawnLocation, FRotator::ZeroRotator, Params) : nullptr;
	if (!Robot) { return Respond(false, LOCTEXT("SpawnFailed", "机器人暂时无法出场，未扣除金币。")); }
	if (!Warehouse->GetStorageComponent()->RemoveItem({EItemType::Coin, Price}))
	{
		Robot->Destroy();
		return false;
	}
	Workers.Add(Robot);
	Robot->OnDestroyed.AddUniqueDynamic(this, &UDemoRunComponent::WorkerDestroyed);
	ApplyCoreBonus(Robot);
	Robot->SpawnDefaultController();
	GetWorld()->GetGameInstance()->GetSubsystem<UMiningGameSubsystem>()->GetPlayerData()->UsePopulation(1);
	if (bCarrier) { ++PurchasedCarriers; } else { ++PurchasedOreBuddies; }
	UE_LOG(LogTemp, Display, TEXT("[DemoRun] Bought %s at %.1fs"), bCarrier ? TEXT("Carrier") : TEXT("OreBuddy"), GetWorld()->GetTimeSeconds() - StartedAt);
	InventoryChanged();
	return Respond(true, LOCTEXT("RobotBought", "机器人已从中心出场。Carrier 优先执行仓库订单；OreBuddy 自动采矿并送回仓库。"));
}

void UDemoRunComponent::WorkerDestroyed(AActor* Worker)
{
	Workers.Remove(Cast<APawn>(Worker));
	GetWorld()->GetGameInstance()->GetSubsystem<UMiningGameSubsystem>()->GetPlayerData()->ReleasePopulation(1);
	OnRunChanged.Broadcast();
}

bool UDemoRunComponent::IsFormUnlocked(EPlayerTransformationForm Form) const
{
	return !IsEnabled() || (Form != EPlayerTransformationForm::Gunner && Form != EPlayerTransformationForm::Guren)
		|| (Form == EPlayerTransformationForm::Gunner ? bGunnerUnlocked : bGurenUnlocked);
}

void UDemoRunComponent::ApplyCoreBonus(APawn* Pawn) const
{
	if (IsEnabled() && IsValid(Pawn))
	{
		if (UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>())
		{
			Combat->SetAttributeBonus(CoreBonus);
		}
		OnUnitReady.Broadcast(Pawn);
	}
}

bool UDemoRunComponent::ChangeForm(EPlayerTransformationForm Form)
{
	if (!IsFormUnlocked(Form)) { return Respond(false, LOCTEXT("FormLocked", "该形态尚未解锁，请先用仓库铁锭解锁。")); }
	for (TActorIterator<APlayerTransformZone> It(GetWorld()); It; ++It)
	{
		if (It->TrySelectForm(Cast<APlayerController>(GetOwner()), Form))
		{
			return Respond(true, LOCTEXT("FormChanged", "现场幻化完成。携带中的资源会放在脚边；核心升级保留。"));
		}
	}
	return Respond(false, LOCTEXT("FormFailed", "当前无法幻化，请先结束正在执行的动作。"));
}

void UDemoRunComponent::InventoryChanged()
{
	OnRunChanged.Broadcast();
}

bool UDemoRunComponent::BeginBossChallenge()
{
	if (Phase == EDemoPhase::BossChallenge || IsFinished()) { return Respond(false, LOCTEXT("BossAlready", "当前挑战正在进行或已经胜利。")); }
	if (Available(EItemType::Coin) < BossCoinCost || Available(EItemType::IronIngot) < BossIngotCost)
	{
		return Respond(false, FText::Format(LOCTEXT("BossCost", "召唤需要仓库可用 {0} 金币和 {1} 铁锭，一次交齐；预留订单的库存不可使用。"), FText::AsNumber(BossCoinCost), FText::AsNumber(BossIngotCost)));
	}
	UClass* Class = BossClass.LoadSynchronous();
	if (!Class) { return Respond(false, LOCTEXT("BossAssetMissing", "超级铁矿资产未就绪，未扣除物资。")); }
	FVector Location = BossLocation;
	FHitResult Ground;
	if (GetWorld()->LineTraceSingleByChannel(Ground, Location + FVector(0,0,2000), Location - FVector(0,0,1000), ECC_WorldStatic)) { Location.Z = Ground.ImpactPoint.Z; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	BossTarget = GetWorld()->SpawnActor<AMineableOre>(Class, Location, FRotator::ZeroRotator, Params);
	if (!BossTarget) { return Respond(false, LOCTEXT("BossSpawnFailed", "挑战目标生成失败，未扣除物资。")); }
	UResourceStorageComponent* Storage = Warehouse->GetStorageComponent();
	if (!Storage->RemoveItem({EItemType::Coin, BossCoinCost})) { BossTarget->Destroy(); BossTarget = nullptr; return false; }
	if (!Storage->RemoveItem({EItemType::IronIngot, BossIngotCost}))
	{
		Storage->AddItem({EItemType::Coin, BossCoinCost});
		BossTarget->Destroy(); BossTarget = nullptr; return false;
	}
	BossTarget->FindComponentByClass<UHealthComponent>()->OnDamageResolved.AddUniqueDynamic(this, &UDemoRunComponent::TargetDamaged);
	BossTarget->OnDestroyed.AddUniqueDynamic(this, &UDemoRunComponent::BossDestroyed);
	BossStartedAt = GetWorld()->GetTimeSeconds();
	RemainingSeconds = ChallengeDuration;
	++BossAttempts;
	Phase = EDemoPhase::BossChallenge;
	GetWorld()->GetTimerManager().SetTimer(ClockHandle, this, &UDemoRunComponent::AdvanceClock, 1.f, true);
	return Respond(true, FText::Format(LOCTEXT("BossReady", "超级铁矿已在下层中央出现！{0} 秒内击破即可获胜。超时只结束本次挑战，可重新付费召唤。"), FText::AsNumber(ChallengeDuration)));
}

void UDemoRunComponent::EndBossAttempt()
{
	Phase = EDemoPhase::Production;
	RemainingSeconds = 0.f;
	GetWorld()->GetTimerManager().ClearTimer(ClockHandle);
	if (IsValid(BossTarget))
	{
		BossTarget->FindComponentByClass<UHealthComponent>()->OnDamageResolved.RemoveDynamic(this, &UDemoRunComponent::TargetDamaged);
		BossTarget->OnDestroyed.RemoveDynamic(this, &UDemoRunComponent::BossDestroyed);
		BossTarget->Destroy();
	}
	BossTarget = nullptr;
	Respond(false, LOCTEXT("BossExpired", "本次超级铁矿挑战结束。矿区继续生产，机器人和升级保留；准备好后可再次支付物资召唤。"));
}

void UDemoRunComponent::BossDestroyed(AActor* Actor)
{
	if (Phase != EDemoPhase::BossChallenge || Actor != BossTarget) { return; }
	// Ore can destroy itself in its damage listener before ours is invoked.
	// Only actual depletion before the deadline counts; external removal does not.
	if (BossTarget->IsDestroyed() && GetWorld()->GetTimeSeconds() - BossStartedAt < ChallengeDuration)
	{
		Finish();
	}
	else { EndBossAttempt(); }
}

void UDemoRunComponent::TargetDamaged(const FCombatDamageRequest& Request, const FCombatDamageResult& Result)
{
	if (Phase != EDemoPhase::BossChallenge || Request.Target != BossTarget) { return; }
	// Check the actual deadline too: a shot between timer callbacks must not win late.
	if (GetWorld()->GetTimeSeconds() - BossStartedAt >= ChallengeDuration) { EndBossAttempt(); return; }
	if (Result.CurrentHealth <= 0.f) { Finish(); }
	else { OnRunChanged.Broadcast(); }
}

void UDemoRunComponent::ConfigureFormCost(EPlayerTransformationForm Form, const TArray<FItemStack>& Cost)
{
	FormCosts.Add(Form, Cost);
}

int32 UDemoRunComponent::GetReserveMagazines() const
{
	const UAmmoInventoryComponent* Account = GetOwner()->FindComponentByClass<UAmmoInventoryComponent>();
	return Account ? Account->GetAvailableMagazines() : 0;
}

FText UDemoRunComponent::GetStatusText() const
{
	return FText::Format(LOCTEXT("FreeStatus", "原矿 {0}   铁锭 {1}   金币 {2}\n机器人 {3}/3"),
		FText::AsNumber(Available(EItemType::IronOre)), FText::AsNumber(Available(EItemType::IronIngot)), FText::AsNumber(Available(EItemType::Coin)),
		FText::AsNumber(Workers.Num()));
}

FText UDemoRunComponent::GetObjectiveText() const
{
	if (IsFinished()) { return LOCTEXT("BossWonHUD", "挑战胜利 · 已获得天赋点" ); }
	if (Phase == EDemoPhase::BossChallenge && IsValid(BossTarget))
	{
		return FText::Format(LOCTEXT("BossFightHUD", "超级铁矿 {0} / {1}\n剩余 {2} 秒"), FText::AsNumber(FMath::CeilToInt(BossTarget->GetCurrentHealth())), FText::AsNumber(BossTarget->GetMaxHealth()), FText::AsNumber(FMath::CeilToInt(RemainingSeconds)));
	}
	return FText::Format(LOCTEXT("BossGoalHUD", "目标 · 击破超级铁矿\n召唤：{0} 金币 + {1} 铁锭 [Tab]"), FText::AsNumber(BossCoinCost), FText::AsNumber(BossIngotCost));
}

FText UDemoRunComponent::GetGuideText() const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (const AHaulerCharacter* Carrier = Cast<AHaulerCharacter>(Pawn))
	{
		return FText::Format(LOCTEXT("CarrierGuide", "{0}\n{1}"), GetNextActionText(), Carrier->GetPlayerCargoDescription());
	}
	if (Phase == EDemoPhase::Briefing) { return LOCTEXT("BriefBoss", "选择召唤师 → 开始本局" ); }
	if (Phase == EDemoPhase::BossChallenge) { return LOCTEXT("BossGuide", "沿引导线攻击超级铁矿 · Q 攻击 / R 技能" ); }
	return GetNextActionText();
}

FText UDemoRunComponent::GetResultText() const
{
	if (!IsFinished()) { return FText::GetEmpty(); }
	return FText::Format(LOCTEXT("BossResult", "超级铁矿击破 · 矿区挑战胜利\n本局经营 {0} 秒 · 第 {1} 次挑战成功\n自动 Carrier {2} 台 / OreBuddy {3} 台\n你可以继续经营，或从终端重新开始。"),
		FText::AsNumber(FMath::RoundToInt(FinishedAt - StartedAt)), FText::AsNumber(BossAttempts),
		FText::AsNumber(PurchasedCarriers), FText::AsNumber(PurchasedOreBuddies));
}

#undef LOCTEXT_NAMESPACE
