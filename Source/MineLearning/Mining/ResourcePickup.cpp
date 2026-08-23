#include "ResourcePickup.h"

AResourcePickup::AResourcePickup()
{
}

void AResourcePickup::InitializeResource(
	EResourceType InType,
	int32 InAmount,
	const TArray<TObjectPtr<UStaticMesh>>& InDropMeshes,
	float InDropMeshScale)
{
	(void)InType;
	FItemStack Stack;
	Stack.ItemType = EItemType::IronOre;
	Stack.Amount = InAmount;
	InitializeItem(Stack, InDropMeshes, InDropMeshScale);
}

EResourceType AResourcePickup::GetResourceType() const
{
	return EResourceType::Iron;
}
