#include "ItemLogisticsLibrary.h"

#include "ItemReceiver.h"
#include "OreProcessorMachine.h"
#include "SellStation.h"
#include "WarehouseDepot.h"
#include "Components/SceneComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/SoftObjectPath.h"

namespace ItemLogistics
{
	static const FSoftObjectPath ItemRulesPath(
		TEXT("/Game/MineLearning/Mining/Logistics/DT_ItemRules.DT_ItemRules"));
}

UDataTable* UItemLogisticsLibrary::GetRulesTable()
{
	return Cast<UDataTable>(ItemLogistics::ItemRulesPath.TryLoad());
}

FName UItemLogisticsLibrary::GetRuleRowName(EItemType ItemType)
{
	const UEnum* ItemTypeEnum = StaticEnum<EItemType>();
	return ItemTypeEnum
		? FName(ItemTypeEnum->GetNameStringByValue(static_cast<int64>(ItemType)))
		: NAME_None;
}

bool UItemLogisticsLibrary::GetItemRule(const FItemStack& Item, FItemRuleRow& OutRule)
{
	if (!Item.IsValid())
	{
		return false;
	}

	UDataTable* RulesTable = GetRulesTable();
	if (!RulesTable)
	{
		return false;
	}

	const FItemRuleRow* Rule = RulesTable->FindRow<FItemRuleRow>(
		GetRuleRowName(Item.ItemType), TEXT("ItemLogistics"), false);
	if (!Rule)
	{
		return false;
	}

	OutRule = *Rule;
	return true;
}

EItemCategory UItemLogisticsLibrary::GetItemCategory(EItemType ItemType)
{
	FItemStack LookupItem;
	LookupItem.ItemType = ItemType;
	LookupItem.Amount = 1;

	FItemRuleRow Rule;
	return GetItemRule(LookupItem, Rule) ? Rule.Category : EItemCategory::Misc;
}

int32 UItemLogisticsLibrary::GetUnitSellPrice(EItemType ItemType)
{
	FItemStack LookupItem;
	LookupItem.ItemType = ItemType;
	LookupItem.Amount = 1;

	FItemRuleRow Rule;
	return GetItemRule(LookupItem, Rule) ? FMath::Max(Rule.UnitSellPrice, 0) : 0;
}

bool UItemLogisticsLibrary::TryGetReceiverType(
	AActor* Receiver,
	EItemReceiverType& OutReceiverType)
{
	if (!IsValid(Receiver)
		|| !Receiver->GetClass()->ImplementsInterface(UItemReceiver::StaticClass()))
	{
		return false;
	}

	if (const IItemReceiver* NativeReceiver = Cast<IItemReceiver>(Receiver))
	{
		OutReceiverType = NativeReceiver->GetItemReceiverType_Implementation();
		return true;
	}

	OutReceiverType = IItemReceiver::Execute_GetItemReceiverType(Receiver);
	return true;
}

bool UItemLogisticsLibrary::CanReceiverAcceptItem(
	AActor* Receiver,
	const FItemStack& Item)
{
	if (!IsValid(Receiver)
		|| !Receiver->GetClass()->ImplementsInterface(UItemReceiver::StaticClass()))
	{
		return false;
	}

	if (const IItemReceiver* NativeReceiver = Cast<IItemReceiver>(Receiver))
	{
		return NativeReceiver->CanAcceptItem_Implementation(Item);
	}

	return IItemReceiver::Execute_CanAcceptItem(Receiver, Item);
}

bool UItemLogisticsLibrary::DeliverItemToReceiver(
	AActor* Receiver,
	const FItemStack& Item)
{
	if (!IsValid(Receiver)
		|| !Receiver->GetClass()->ImplementsInterface(UItemReceiver::StaticClass()))
	{
		return false;
	}

	if (IItemReceiver* NativeReceiver = Cast<IItemReceiver>(Receiver))
	{
		return NativeReceiver->AcceptItem_Implementation(Item);
	}

	return IItemReceiver::Execute_AcceptItem(Receiver, Item);
}

FVector UItemLogisticsLibrary::GetReceiverDeliveryLocation(AActor* Receiver)
{
	if (const AOreProcessorMachine* Processor = Cast<AOreProcessorMachine>(Receiver))
	{
		return Processor->GetDeliveryPointWorldTransform().GetLocation();
	}
	if (const ASellStation* Seller = Cast<ASellStation>(Receiver))
	{
		return Seller->GetRobotApproachPoint()->GetComponentLocation();
	}
	if (const AWarehouseDepot* Warehouse = Cast<AWarehouseDepot>(Receiver))
	{
		return Warehouse->GetDeliveryPointWorldTransform().GetLocation();
	}
	return IsValid(Receiver) ? Receiver->GetActorLocation() : FVector::ZeroVector;
}

AActor* UItemLogisticsLibrary::FindNearbyPlayerMachine(const AActor* Carrier, const FItemStack& Item)
{
	if (!IsValid(Carrier) || !Carrier->GetWorld() || !Item.IsValid())
	{
		return nullptr;
	}
	AActor* Nearest = nullptr;
	float BestDistance = FMath::Square(260.f);
	for (TActorIterator<AActor> It(Carrier->GetWorld()); It; ++It)
	{
		const bool bCompatible = (It->IsA<AOreProcessorMachine>() && Item.ItemType == EItemType::IronOre)
			|| (It->IsA<ASellStation>() && GetUnitSellPrice(Item.ItemType) > 0);
		if (!bCompatible || It->IsActorBeingDestroyed()) { continue; }
		const FVector Point = GetReceiverDeliveryLocation(*It);
		const float Distance = FVector::DistSquared2D(Carrier->GetActorLocation(), Point);
		if (Distance <= BestDistance && FMath::Abs(Carrier->GetActorLocation().Z - Point.Z) <= 200.f)
		{
			BestDistance = Distance;
			Nearest = *It;
		}
	}
	return Nearest;
}

AActor* UItemLogisticsLibrary::ResolveDestination(
	const UObject* WorldContextObject,
	const FItemStack& Item,
	const FVector& SearchOrigin)
{
	if (!WorldContextObject || !Item.IsValid())
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	FItemRuleRow Rule;
	if (!World || !GetItemRule(Item, Rule))
	{
		return nullptr;
	}

	for (EItemReceiverType ReceiverType : Rule.ReceiverPriority)
	{
		AActor* NearestReceiver = nullptr;
		float NearestDistanceSq = TNumericLimits<float>::Max();

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Candidate = *It;
			EItemReceiverType CandidateType = EItemReceiverType::Processor;
			if (!IsValid(Candidate)
				|| Candidate->IsActorBeingDestroyed()
				|| !TryGetReceiverType(Candidate, CandidateType)
				|| CandidateType != ReceiverType
				|| !CanReceiverAcceptItem(Candidate, Item))
			{
				continue;
			}

			const float DistanceSq = FVector::DistSquared(SearchOrigin, Candidate->GetActorLocation());
			if (DistanceSq < NearestDistanceSq)
			{
				NearestDistanceSq = DistanceSq;
				NearestReceiver = Candidate;
			}
		}

		if (NearestReceiver)
		{
			return NearestReceiver;
		}
	}

	return nullptr;
}
