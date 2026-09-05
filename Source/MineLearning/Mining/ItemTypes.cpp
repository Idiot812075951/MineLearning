#include "ItemTypes.h"

#include "Engine/StaticMesh.h"

namespace
{
	float GetTargetMaxDimension(EItemType ItemType)
	{
		return ItemType == EItemType::Coin
			? MineLearningItemVisual::CoinMaxDimensionCm
			: MineLearningItemVisual::StandardMaxDimensionCm;
	}
}

float MineLearningItemVisual::GetUniformScale(const UStaticMesh* Mesh)
{
	return GetUniformScale(Mesh, EItemType::IronOre);
}

float MineLearningItemVisual::GetUniformScale(
	const UStaticMesh* Mesh,
	EItemType ItemType)
{
	if (!IsValid(Mesh))
	{
		return 1.0f;
	}

	const FVector MeshSize = Mesh->GetBounds().BoxExtent * 2.0f;
	const float MaxDimension = MeshSize.GetAbsMax();
	return MaxDimension > UE_SMALL_NUMBER
		? GetTargetMaxDimension(ItemType) / MaxDimension
		: 1.0f;
}

FVector MineLearningItemVisual::GetRelativeScale(
	const UStaticMesh* Mesh,
	const FVector& ParentWorldScale)
{
	return GetRelativeScale(Mesh, ParentWorldScale, EItemType::IronOre);
}

FVector MineLearningItemVisual::GetRelativeScale(
	const UStaticMesh* Mesh,
	const FVector& ParentWorldScale,
	EItemType ItemType)
{
	const float UniformScale = GetUniformScale(Mesh, ItemType);
	return FVector(
		UniformScale / FMath::Max(FMath::Abs(ParentWorldScale.X), UE_SMALL_NUMBER),
		UniformScale / FMath::Max(FMath::Abs(ParentWorldScale.Y), UE_SMALL_NUMBER),
		UniformScale / FMath::Max(FMath::Abs(ParentWorldScale.Z), UE_SMALL_NUMBER));
}

FVector MineLearningItemVisual::GetWorldSize(const UStaticMesh* Mesh)
{
	return GetWorldSize(Mesh, EItemType::IronOre);
}

FVector MineLearningItemVisual::GetWorldSize(
	const UStaticMesh* Mesh,
	EItemType ItemType)
{
	return IsValid(Mesh)
		? Mesh->GetBounds().BoxExtent * 2.0f * GetUniformScale(Mesh, ItemType)
		: FVector::ZeroVector;
}
