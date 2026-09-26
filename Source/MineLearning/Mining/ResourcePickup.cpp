#include "ResourcePickup.h"
#include "Components/StaticMeshComponent.h"

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
	if (Mesh && !FinishVariants.IsEmpty())
	{
		Mesh->SetMaterial(0, FinishVariants[FMath::RandHelper(FinishVariants.Num())]);
	}
}

EResourceType AResourcePickup::GetResourceType() const
{
	return EResourceType::Iron;
}
