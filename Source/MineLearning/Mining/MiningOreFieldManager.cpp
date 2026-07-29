#include "MiningOreFieldManager.h"

#include "Kismet/GameplayStatics.h"
#include "MineableOre.h"
#include "MiningOreSpawnPoint.h"
#include "OreDefinitionDataAsset.h"

AMiningOreFieldManager::AMiningOreFieldManager()
{
	PrimaryActorTick.bCanEverTick = false;
	OreClass = AMineableOre::StaticClass();
}

void AMiningOreFieldManager::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnBatchOnBeginPlay)
	{
		SpawnBatch();
	}
}

void AMiningOreFieldManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearCurrentBatch();
	Super::EndPlay(EndPlayReason);
}

int32 AMiningOreFieldManager::SpawnBatch()
{
	ClearCurrentBatch();
	bBatchClearedBroadcast = false;

	if (!OreClass)
	{
		return 0;
	}

	TArray<AActor*> SpawnPointActors;
	UGameplayStatics::GetAllActorsOfClass(this, AMiningOreSpawnPoint::StaticClass(), SpawnPointActors);
	if (SpawnPointActors.IsEmpty())
	{
		return 0;
	}

	TArray<AMiningOreSpawnPoint*> SpawnPoints;
	SpawnPoints.Reserve(SpawnPointActors.Num());
	for (AActor* SpawnPointActor : SpawnPointActors)
	{
		if (AMiningOreSpawnPoint* SpawnPoint = Cast<AMiningOreSpawnPoint>(SpawnPointActor))
		{
			SpawnPoints.Add(SpawnPoint);
		}
	}

	if (SpawnPoints.IsEmpty())
	{
		return 0;
	}

	for (int32 LastIndex = SpawnPoints.Num() - 1; LastIndex > 0; --LastIndex)
	{
		SpawnPoints.Swap(LastIndex, FMath::RandRange(0, LastIndex));
	}

	int32 SpawnPointIndex = 0;
	for (const FMiningOreBatchEntry& BatchEntry : BatchEntries)
	{
		if (!BatchEntry.OreDefinition || BatchEntry.Count <= 0)
		{
			continue;
		}

		for (int32 OreIndex = 0; OreIndex < BatchEntry.Count; ++OreIndex)
		{
			AMiningOreSpawnPoint* SpawnPoint = SpawnPoints[SpawnPointIndex % SpawnPoints.Num()];
			++SpawnPointIndex;

			const FTransform SpawnTransform = SpawnPoint->GetActorTransform();
			AMineableOre* Ore = GetWorld()->SpawnActorDeferred<AMineableOre>(
				OreClass,
				SpawnTransform,
				this,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn
			);

			if (!Ore)
			{
				continue;
			}

			Ore->SetOreDefinition(BatchEntry.OreDefinition);
			UGameplayStatics::FinishSpawningActor(Ore, SpawnTransform);
			Ore->OnOreDepleted.AddDynamic(this, &AMiningOreFieldManager::HandleOreDepleted);
			ActiveOres.Add(Ore);
		}
	}

	RemainingOreCount = ActiveOres.Num();
	bBatchActive = RemainingOreCount > 0;
	return RemainingOreCount;
}

void AMiningOreFieldManager::ClearCurrentBatch()
{
	bClearingCurrentBatch = true;
	bBatchActive = false;

	for (AMineableOre* Ore : ActiveOres)
	{
		if (IsValid(Ore))
		{
			Ore->OnOreDepleted.RemoveDynamic(this, &AMiningOreFieldManager::HandleOreDepleted);
			Ore->Destroy();
		}
	}

	ActiveOres.Empty();
	RemainingOreCount = 0;
	bClearingCurrentBatch = false;
	bBatchClearedBroadcast = false;
}

void AMiningOreFieldManager::HandleOreDepleted(AMineableOre* DepletedOre)
{
	if (bClearingCurrentBatch || !bBatchActive || !DepletedOre)
	{
		return;
	}

	DepletedOre->OnOreDepleted.RemoveDynamic(this, &AMiningOreFieldManager::HandleOreDepleted);
	ActiveOres.Remove(DepletedOre);
	RemainingOreCount = ActiveOres.Num();

	if (RemainingOreCount == 0)
	{
		CompleteCurrentBatch();
	}
}

void AMiningOreFieldManager::CompleteCurrentBatch()
{
	if (!bBatchActive || bBatchClearedBroadcast)
	{
		return;
	}

	bBatchActive = false;
	bBatchClearedBroadcast = true;
	OnOreBatchCleared.Broadcast();
}
