#include "MiningGameSubsystem.h"

#include "MiningPlayerData.h"

void UMiningGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PlayerData = NewObject<UMiningPlayerData>(this);
}
