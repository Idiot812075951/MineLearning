#include "ItemTypes.h"

#include "Engine/StaticMesh.h"

float MineLearningItemVisual::GetUniformScale(const UStaticMesh* Mesh)
{
	if (!IsValid(Mesh))
	{
		return 1.0f;
	}

	const FVector MeshSize = Mesh->GetBounds().BoxExtent * 2.0f;
	const float MaxDimension = MeshSize.GetAbsMax();
	return MaxDimension > UE_SMALL_NUMBER
		? StandardMaxDimensionCm / MaxDimension
		: 1.0f;
}

FVector MineLearningItemVisual::GetRelativeScale(
	const UStaticMesh* Mesh,
	const FVector& ParentWorldScale)
{
	const float UniformScale = GetUniformScale(Mesh);
	return FVector(
		UniformScale / FMath::Max(FMath::Abs(ParentWorldScale.X), UE_SMALL_NUMBER),
		UniformScale / FMath::Max(FMath::Abs(ParentWorldScale.Y), UE_SMALL_NUMBER),
		UniformScale / FMath::Max(FMath::Abs(ParentWorldScale.Z), UE_SMALL_NUMBER));
}

FVector MineLearningItemVisual::GetWorldSize(const UStaticMesh* Mesh)
{
	return IsValid(Mesh)
		? Mesh->GetBounds().BoxExtent * 2.0f * GetUniformScale(Mesh)
		: FVector::ZeroVector;
}
