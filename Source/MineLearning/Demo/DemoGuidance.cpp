#include "DemoRunComponent.h"

#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/AI/HaulerCharacter.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"

#include "MineLearning/Combat/HealthComponent.h"
#include "MineLearning/TransformationGuard.h"

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
	const ITransformationGuard* Guard = Cast<ITransformationGuard>(PC);
	const bool bCanTransform = !Guard || Guard->CanTransform();
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
		return Command(EDemoCommand::Start, LOCTEXT("Start", "选择身份，点击「开始本局」"));
	}
	if (Phase == EDemoPhase::BossChallenge && IsValid(BossTarget))
	{
		if (const AGunnerCharacter* Gunner = Cast<AGunnerCharacter>(Pawn); Gunner && Gunner->GetCurrentAmmo() == 0 && GetReserveMagazines() == 0)
		{
			return Command(EDemoCommand::BuyMagazine, LOCTEXT("BuyAmmo", "[Tab] 购买弹匣 → [R] 装填"));
		}
		return Location(BossTarget->GetActorLocation(), LOCTEXT("FightBoss", "沿引导线攻击超级铁矿"));
	}
	if (!Warehouse) { return Guide; }
	const UResourceCarryComponent* Carry = Pawn->FindComponentByClass<UResourceCarryComponent>();
	const AHaulerCharacter* Carrier = Cast<AHaulerCharacter>(Pawn);
	if (Carry && !Carry->IsEmpty())
	{
		if (AActor* Machine = UItemLogisticsLibrary::FindNearbyPlayerMachine(Pawn, Carry->GetCurrentItem()))
		{
			return Location(UItemLogisticsLibrary::GetReceiverDeliveryLocation(Machine),
				Carrier ? LOCTEXT("DirectCarrierMachine", "[左键] 交付货物") : LOCTEXT("DirectMachine", "[E] 交付货物"));
		}
	}
	if (Carrier && Carry && !Carry->IsEmpty())
	{
		FVector Destination;
		if (Carrier->GetPlayerDeliveryLocation(Destination))
		{
			return Location(Destination, LOCTEXT("Deliver", "沿引导线送货 → [左键] 交付"));
		}
	}
	if (Pawn->IsA<AMiningCompanionCharacter>() && Carry && Carry->IsFull())
	{
		return Location(Warehouse->GetDeliveryPointWorldTransform().GetLocation(), LOCTEXT("DepositOrProcess", "货舱已满 → 送往加工机或仓库"));
	}
	const int32 Coins = Available(EItemType::Coin);
	const int32 Ingots = Available(EItemType::IronIngot);
	if (PurchasedCarriers == 0 && Coins >= 4)
	{
		return Command(EDemoCommand::BuyCarrier, LOCTEXT("BuyCarrier", "[Tab] 自动 Carrier · 4 金币"));
	}
	if (PurchasedOreBuddies == 0 && Coins >= 8)
	{
		return Command(EDemoCommand::BuyOreBuddy, LOCTEXT("BuyBuddy", "[Tab] 自动 OreBuddy · 8 金币"));
	}
	const bool bWorkersReady = PurchasedCarriers > 0 && PurchasedOreBuddies > 0;
	if (bCanTransform && !bGunnerUnlocked && Available(EItemType::IronOre) >= 4)
	{
		return Command(EDemoCommand::UnlockGunner, LOCTEXT("UnlockGunner", "[Tab] 购买 Gunner · 4 原矿"));
	}
	if (bCanTransform && bGunnerUnlocked && GetReserveMagazines() < 14 && Ingots > BossIngotCost)
	{
		return Command(EDemoCommand::BuyMagazine, LOCTEXT("PrepareAmmo", "[Tab] 购买备用弹匣 · 1 铁锭"));
	}
	if (Coins >= BossCoinCost && Ingots >= BossIngotCost)
	{
		return Command(EDemoCommand::SubmitMaterials, LOCTEXT("Summon", "[Tab] 召唤超级铁矿"));
	}
	if (Available(EItemType::IronOre) >= 2)
	{
		return Command(EDemoCommand::ProcessFour, LOCTEXT("Process", "[Tab] 加工 · 2 原矿 → 1 铁锭"));
	}
	const bool bNeedCoins = !bWorkersReady || Coins < BossCoinCost;
	if (bNeedCoins && Ingots > 0)
	{
		return Command(EDemoCommand::SellTwo, LOCTEXT("Sell", "[Tab] 出售 · 1 铁锭 → 2 金币"));
	}
	AItemPickup* Pickup = nullptr;
	if (!bCanTransform)
	{
		Guide.Instruction = LOCTEXT("CoordinateWorkers", "AI 自动生产 · [Tab] 下单 · 商店购买专属升级");
		return Guide;
	}
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
			return Command(EDemoCommand::Carrier, LOCTEXT("Carrier", "[Tab] 幻化 Carrier → 运货"));
		}
		return Location(Pickup->GetActorLocation(), Carrier
			? LOCTEXT("CollectCarrier", "沿引导线取货 → [左键]")
			: LOCTEXT("CollectBuddy", "沿引导线靠近原矿 → [R] 拾取"));
	}
	if (!Pawn->IsA<AMiningCompanionCharacter>())
	{
		return Command(EDemoCommand::OreBuddy, LOCTEXT("Buddy", "[Tab] 幻化 OreBuddy → 采矿"));
	}
	if (Carry && !Carry->IsEmpty())
	{
		return Location(Warehouse->GetDeliveryPointWorldTransform().GetLocation(), LOCTEXT("PartialDeposit", "沿引导线送回仓库"));
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
		return Location(Ore->GetActorLocation(), LOCTEXT("Mine", "靠近矿石 → [左键] 钻采 / [R] 拾取"));
	}
	Guide.Instruction = LOCTEXT("Respawn", "矿点恢复中 · 可先加工或出售" );
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
