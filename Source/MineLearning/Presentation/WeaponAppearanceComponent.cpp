#include "WeaponAppearanceComponent.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInterface.h"

UWeaponAppearanceComponent::UWeaponAppearanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	WeaponMaterials.Add(TEXT("GoldenAK"), TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MineLearning/Roguelite/Materials/M_GoldenAK.M_GoldenAK"))));
}

void UWeaponAppearanceComponent::BeginPlay()
{
	Super::BeginPlay();
	TInlineComponentArray<UMeshComponent*> Meshes(GetOwner());
	for (UMeshComponent* Mesh : Meshes)
	{
		if (!MeshNames.Contains(Mesh->GetFName()))
		{
			continue;
		}
		for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
		{
			FWeaponMaterialSlot& Slot = Slots.AddDefaulted_GetRef();
			Slot.Mesh = Mesh;
			Slot.Index = Index;
			Slot.Original = Mesh->GetMaterial(Index);
		}
	}
	Combat = GetOwner()->FindComponentByClass<UCombatComponent>();
	if (Combat)
	{
		Combat->OnAttributesChanged.AddUniqueDynamic(this, &UWeaponAppearanceComponent::RefreshAppearance);
	}
	RefreshAppearance();
}

void UWeaponAppearanceComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Combat)
	{
		Combat->OnAttributesChanged.RemoveDynamic(this, &UWeaponAppearanceComponent::RefreshAppearance);
	}
	Super::EndPlay(Reason);
}

void UWeaponAppearanceComponent::RefreshAppearance()
{
	const FName Style = Combat ? Combat->GetModifiers().WeaponStyle : NAME_None;
	if (Style == AppliedStyle)
	{
		return;
	}
	if (GetOwner()->ActorHasTag(TEXT("Phantom")))
	{
		return;
	}
	const TSoftObjectPtr<UMaterialInterface>* Material = WeaponMaterials.Find(Style);
	UMaterialInterface* Loaded = Material ? Material->LoadSynchronous() : nullptr;
	for (const FWeaponMaterialSlot& Slot : Slots)
	{
		if (IsValid(Slot.Mesh))
		{
			Slot.Mesh->SetMaterial(Slot.Index, Loaded ? Loaded : Slot.Original.Get());
		}
	}
	AppliedStyle = Style;
}
