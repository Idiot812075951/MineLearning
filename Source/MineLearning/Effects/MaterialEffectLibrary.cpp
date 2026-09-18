#include "MaterialEffectLibrary.h"

#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UMaterialInstanceDynamic* UMaterialEffectLibrary::CreateIsolatedMaterialInstance(UMeshComponent* Mesh, int32 ElementIndex)
{
	if (!IsValid(Mesh) || ElementIndex < 0 || ElementIndex >= Mesh->GetNumMaterials())
	{
		return nullptr;
	}
	UMaterialInterface* Source = Mesh->GetMaterial(ElementIndex);
	UMaterialInterface* Parent = Source;
	while (const UMaterialInstanceDynamic* DynamicParent = Cast<UMaterialInstanceDynamic>(Parent))
	{
		Parent = DynamicParent->Parent;
	}
	if (!Parent)
	{
		return nullptr;
	}
	// Preserve the constant-instance parent (including static switches), then snapshot
	// effective runtime parameters. A MID cannot be used as another MID's parent.
	UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Parent, Mesh);
	Instance->CopyMaterialUniformParameters(Source);
	Mesh->SetMaterial(ElementIndex, Instance);
	return Instance;
}
