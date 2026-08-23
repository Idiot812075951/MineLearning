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

FTransform AResourceDepot::GetDeliveryPointWorldTransform() const
{
	// Runtime warehouse Blueprints already expose an authored dock point. Prefer
	// that component so AI, door interaction, and level art all share one source
	// of truth instead of maintaining a second hidden receiver transform.
	TInlineComponentArray<USceneComponent*> SceneComponents(this);
	for (const USceneComponent* Component : SceneComponents)
	{
		if (Component && Component->GetFName() == TEXT("P_Warehouse_DockPoint"))
		{
			return Component->GetComponentTransform();
		}
	}

	return DeliveryPoint * GetActorTransform();
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

EItemReceiverType AResourceDepot::GetItemReceiverType_Implementation() const
{
	return EItemReceiverType::Warehouse;
}

bool AResourceDepot::CanAcceptItem_Implementation(const FItemStack& Item) const
{
	return StorageComponent && StorageComponent->CanAddItem(Item);
}

bool AResourceDepot::AcceptItem_Implementation(const FItemStack& Item)
{
	return StorageComponent && StorageComponent->AddItem(Item);
}
