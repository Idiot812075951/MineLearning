#include "MiningPlayerData.h"

int32 UMiningPlayerData::AddProcessedOre(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	ProcessedOre += Amount;
	BroadcastPlayerDataChanged();
	return Amount;
}

bool UMiningPlayerData::ConsumeProcessedOre(int32 Amount)
{
	if (Amount <= 0 || ProcessedOre < Amount)
	{
		return false;
	}

	ProcessedOre -= Amount;
	BroadcastPlayerDataChanged();
	return true;
}

bool UMiningPlayerData::CanUsePopulation(int32 Amount) const
{
	return Amount > 0 && PopulationUsed + Amount <= PopulationLimit;
}

bool UMiningPlayerData::UsePopulation(int32 Amount)
{
	if (!CanUsePopulation(Amount))
	{
		return false;
	}

	PopulationUsed += Amount;
	BroadcastPlayerDataChanged();
	return true;
}

int32 UMiningPlayerData::ReleasePopulation(int32 Amount)
{
	if (Amount <= 0 || PopulationUsed <= 0)
	{
		return 0;
	}

	const int32 ReleasedAmount = FMath::Min(Amount, PopulationUsed);
	PopulationUsed -= ReleasedAmount;
	BroadcastPlayerDataChanged();
	return ReleasedAmount;
}

void UMiningPlayerData::BroadcastPlayerDataChanged()
{
	OnPlayerDataChanged.Broadcast();
}
