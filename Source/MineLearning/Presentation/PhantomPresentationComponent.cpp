#include "PhantomPresentationComponent.h"
#include "MineLearning/AI/UnitRetirementComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UPhantomPresentationComponent::UPhantomPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPhantomPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	UMaterialInterface* Hologram = Material.LoadSynchronous();
	if (Hologram)
	{
		TInlineComponentArray<UMeshComponent*> Meshes(GetOwner());
		for (UMeshComponent* Mesh : Meshes)
		{
			// Translucent holograms require the conventional renderer on attachments.
			if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
			{
				StaticMesh->bDisallowNanite = true;
				StaticMesh->MarkRenderStateDirty();
			}
			for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
			{
				UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Hologram, this);
				Mesh->SetMaterial(Index, Instance);
				Materials.Add(Instance);
			}
		}
	}
	Retirement = GetOwner()->FindComponentByClass<UUnitRetirementComponent>();
	if (Retirement) { Retirement->OnRetiring.AddUniqueDynamic(this, &UPhantomPresentationComponent::Retiring); }
}

void UPhantomPresentationComponent::Retiring()
{
	FadeRemaining = Retirement->RetirementDuration;
	SetComponentTickEnabled(true);
}

void UPhantomPresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
	Super::TickComponent(DeltaTime, TickType, Function);
	FadeRemaining = FMath::Max(0.f, FadeRemaining - DeltaTime);
	const float Alpha = FadeRemaining / FMath::Max(0.01f, Retirement->RetirementDuration);
	for (UMaterialInstanceDynamic* Instance : Materials) { Instance->SetScalarParameterValue(TEXT("Visibility"), Alpha); }
	if (FadeRemaining <= 0.f) { SetComponentTickEnabled(false); }
}

void UPhantomPresentationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Retirement) { Retirement->OnRetiring.RemoveDynamic(this, &UPhantomPresentationComponent::Retiring); }
	Super::EndPlay(Reason);
}
