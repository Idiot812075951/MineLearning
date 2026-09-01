#include "ResourcePickup.h"

AResourcePickup::AResourcePickup()
{
}

void AResourcePickup::InitializeResource(
	EResourceType InType,
	int32 InAmount,
	const TArray<TObjectPtr<UStaticMesh>>& InDropMeshes)
{
	(void)InType;
	FItemStack Stack;
	Stack.ItemType = EItemType::IronOre;
	Stack.Amount = InAmount;
	InitializeItem(Stack, InDropMeshes);
}

EResourceType AResourcePickup::GetResourceType() const
{
	return EResourceType::Iron;
}
