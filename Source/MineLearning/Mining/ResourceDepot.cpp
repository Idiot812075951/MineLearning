#include "ResourceDepot.h"

#include "ResourceCarryComponent.h"
#include "ResourceStorageComponent.h"
#include "Components/SceneComponent.h"

AResourceDepot::AResourceDepot()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	StorageComponent = CreateDefaultSubobject<UResourceStorageComponent>(TEXT("ResourceStorageComponent"));
}

int32 AResourceDepot::DepositFromCarry(UResourceCarryComponent* CarryComponent)
{
	if (!CarryComponent || !StorageComponent)
	{
		return 0;
	}

	const int32 OreAmount = CarryComponent->TakeAllOre();
	if (OreAmount <= 0)
	{
		return 0;
	}

	return StorageComponent->AddOre(OreAmount);
}
