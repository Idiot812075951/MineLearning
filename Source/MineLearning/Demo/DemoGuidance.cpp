#include "DemoRunComponent.h"

#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/AI/HaulerCharacter.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"

#include "MineLearning/Combat/HealthComponent.h"

#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Mining/ItemLogisticsLibrary.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "DemoGuidance"

struct FDemoGuidance
{
	FText Instruction;
	EDemoCommand Command = EDemoCommand::Start;
	bool bCommand = false;
	bool bDestination = false;
	FVector Destination = FVector::ZeroVector;
};

FDemoGuidance UDemoRunComponent::ResolveGuidance() const
{
	FDemoGuidance Guide;
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn || IsFinished() || !IsEnabled()) { return Guide; }
	const auto Command = [&Guide](EDemoCommand Action, const FText& Text)
	{
		Guide.bCommand = true;
		Guide.Command = Action;
		Guide.Instruction = Text;
		return Guide;
	};
	const auto Location = [&Guide](FVector Destination, const FText& Text)
	{
		Guide.bDestination = true;
		Guide.Destination = Destination;
		Guide.Instruction = Text;
		return Guide;
	};
	if (Phase == EDemoPhase::Briefing)
	{
		return Command(EDemoCommand::Start, LOCTEXT("Start", "点击高亮的「开始生产」，自由生产不限时。"));
	}
	if (Phase == EDemoPhase::BossChallenge && IsValid(BossTarget))
	{
		if (const AGunnerCharacter* Gunner = Cast<AGunnerCharacter>(Pawn); Gunner && Gunner->GetCurrentAmmo() == 0 && ReserveMagazines == 0)
		{
			return Command(EDemoCommand::BuyMagazine, LOCTEXT("BuyAmmo", "Gunner 无弹药。按 Tab 买弹匣（1 铁锭 / 20 发），再按 R 装填。也可切换 OreBuddy 钻采。"));
		}
		return Location(BossTarget->GetActorLocation(), LOCTEXT("FightBoss", "沿蓝色引导线攻击超级铁矿，在倒计时结束前击破即可获胜。"));
	}
	if (!Warehouse) { return Guide; }
	const UResourceCarryComponent* Carry = Pawn->FindComponentByClass<UResourceCarryComponent>();
	const AHaulerCharacter* Carrier = Cast<AHaulerCharacter>(Pawn);
	if (Carry && !Carry->IsEmpty())
	{
		if (AActor* Machine = UItemLogisticsLibrary::FindNearbyPlayerMachine(Pawn, Carry->GetCurrentItem()))
		{
			return Location(UItemLogisticsLibrary::GetReceiverDeliveryLocation(Machine),
				LOCTEXT("DirectMachine", "已靠近接收设备：按 E 直接交付携带的货物，无需先回仓库或下订单。"));
		}
	}
	if (Carrier && Carry && !Carry->IsEmpty())
	{
		FVector Destination;
		if (Carrier->GetPlayerDeliveryLocation(Destination))
		{
			return Location(Destination, LOCTEXT("Deliver", "沿蓝色引导线送货，到终点附近按 E。未装满也可以交付。"));
		}
	}
	if (Pawn->IsA<AMiningCompanionCharacter>() && Carry && Carry->IsFull())
	{
		return Location(Warehouse->GetDeliveryPointWorldTransform().GetLocation(), LOCTEXT("DepositOrProcess", "货舱已满。可直接送到加工机入料箱按 E 加工；也可沿引导线回仓库自动卸货。"));
	}
	const int32 Coins = Available(EItemType::Coin);
	const int32 Ingots = Available(EItemType::IronIngot);
	if (PurchasedCarriers == 0 && Coins >= 4)
	{
		return Command(EDemoCommand::BuyCarrier, LOCTEXT("BuyCarrier", "已有 4 金币。按 Tab，点击高亮的「自动 Carrier」购买搬运机器人。"));
	}
	if (PurchasedOreBuddies == 0 && Coins >= 8)
	{
		return Command(EDemoCommand::BuyOreBuddy, LOCTEXT("BuyBuddy", "已有 8 金币。按 Tab，点击高亮的「自动 OreBuddy」。"));
	}
	const bool bWorkersReady = PurchasedCarriers > 0 && PurchasedOreBuddies > 0;
	if (!bGunnerUnlocked && Ingots >= 2)
	{
		return Command(EDemoCommand::UnlockGunner, LOCTEXT("UnlockGunner", "可用 2 铁锭解锁 Gunner；枪械需要额外购买弹匣，OreBuddy 钻采无需弹药。"));
	}
	if (bGunnerUnlocked && ReserveMagazines < 14 && Ingots > BossIngotCost)
	{
		return Command(EDemoCommand::BuyMagazine, LOCTEXT("PrepareAmmo", "准备挑战：购买备用弹匣，每个 1 铁锭 / 20 发；纯枪械挑战建议至少备 14 个。"));
	}
	if (Coins >= BossCoinCost && Ingots >= BossIngotCost)
	{
		return Command(EDemoCommand::SubmitMaterials, LOCTEXT("Summon", "准备就绪后，按 Tab 点击「召唤超级铁矿」。召唤才开始挑战倒计时，核心升级不是必需。"));
	}
	if (Available(EItemType::IronOre) >= 2)
	{
		return Command(EDemoCommand::ProcessFour, LOCTEXT("Process", "按 Tab，点击高亮的加工按钮。每 2 原矿产出 1 铁锭，4 原矿产出 2 铁锭。"));
	}
	const bool bNeedCoins = !bWorkersReady || Coins < BossCoinCost;
	if (bNeedCoins && Ingots > 0)
	{
		return Command(EDemoCommand::SellTwo, LOCTEXT("Sell", "按 Tab，点击高亮的出售按钮。每 1 铁锭换 2 金币，金币运回仓库后才能使用。"));
	}
	AItemPickup* Pickup = nullptr;
	float BestDistance = MAX_flt;
	for (TActorIterator<AItemPickup> It(GetWorld()); It; ++It)
	{
		const bool bHaulerWork = It->GetExplicitDeliveryActor() || It->GetItemStack().ItemType != EItemType::IronOre;
		if (It->IsTransportLocked() || !It->GetItemStack().IsValid()) { continue; }
		if ((!bHaulerWork || Carrier) && !It->IsAvailableFor(const_cast<APawn*>(Pawn))) { continue; }
		if (It->GetExplicitDeliveryActor() && !It->HasUsableExplicitDeliveryTarget()) { continue; }
		if (!Carrier && bHaulerWork && PurchasedCarriers > 0) { continue; }
		if (Carrier || bHaulerWork || (Pawn->IsA<AMiningCompanionCharacter>() && It->GetItemStack().ItemType == EItemType::IronOre))
		{
			const float Distance = FVector::DistSquared2D(Pawn->GetActorLocation(), It->GetActorLocation());
			if (Distance < BestDistance) { BestDistance = Distance; Pickup = *It; }
		}
	}
	if (Pickup)
	{
		if (!Carrier && (Pickup->GetExplicitDeliveryActor() || Pickup->GetItemStack().ItemType != EItemType::IronOre))
		{
			return Command(EDemoCommand::Carrier, LOCTEXT("Carrier", "有订单或产物待运输。按 Tab，点击高亮的「幻化 · Carrier」。"));
		}
		return Location(Pickup->GetActorLocation(), Carrier
			? LOCTEXT("CollectCarrier", "沿蓝色引导线取货，靠近终点按 E；不必装满即可送货。")
			: LOCTEXT("CollectBuddy", "沿蓝色引导线靠近散落原矿，按 R 拾取。"));
	}
	if (!Pawn->IsA<AMiningCompanionCharacter>())
	{
		return Command(EDemoCommand::OreBuddy, LOCTEXT("Buddy", "按 Tab，点击高亮的「幻化 · OreBuddy」，继续采矿。设备中的货物会完成加工。"));
	}
	if (Carry && !Carry->IsEmpty())
	{
		return Location(Warehouse->GetDeliveryPointWorldTransform().GetLocation(), LOCTEXT("PartialDeposit", "附近没有待拾取原矿，先沿引导线将携带的矿石送回仓库。"));
	}
	AMineableOre* Ore = nullptr;
	BestDistance = MAX_flt;
	for (TActorIterator<AMineableOre> It(GetWorld()); It; ++It)
	{
		if (It->IsDestroyed()) { continue; }
		const float Distance = FVector::DistSquared2D(Pawn->GetActorLocation(), It->GetActorLocation());
		if (Distance < BestDistance) { BestDistance = Distance; Ore = *It; }
	}
	if (Ore)
	{
		return Location(Ore->GetActorLocation(), LOCTEXT("Mine", "沿蓝色引导线到矿石旁，按 Q 钻采，掉落后按 R 拾取。"));
	}
	Guide.Instruction = LOCTEXT("Respawn", "矿点正在恢复，请稍候；有库存时可先下加工或出售订单。" );
	return Guide;
}

bool UDemoRunComponent::IsRecommendedCommand(EDemoCommand Command) const
{
	const FDemoGuidance Guide = ResolveGuidance();
	return Guide.bCommand && Guide.Command == Command;
}

bool UDemoRunComponent::IsTerminalRecommended() const
{
	return ResolveGuidance().bCommand;
}

FText UDemoRunComponent::GetNextActionText() const
{
	return ResolveGuidance().Instruction;
}

bool UDemoRunComponent::GetGuidanceDestination(FVector& OutLocation) const
{
	const FDemoGuidance Guide = ResolveGuidance();
	OutLocation = Guide.Destination;
	return Guide.bDestination;
}

#undef LOCTEXT_NAMESPACE
