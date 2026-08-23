#include "ItemLogisticsLibrary.h"

#include "ItemReceiver.h"
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
			if (!IsValid(Candidate)
				|| Candidate->IsActorBeingDestroyed()
				|| !Candidate->GetClass()->ImplementsInterface(UItemReceiver::StaticClass())
				|| IItemReceiver::Execute_GetItemReceiverType(Candidate) != ReceiverType
				|| !IItemReceiver::Execute_CanAcceptItem(Candidate, Item))
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
