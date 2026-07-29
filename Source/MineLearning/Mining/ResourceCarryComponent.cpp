#include "ResourceCarryComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UResourceCarryComponent::UResourceCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UResourceCarryComponent::OnRegister()
{
	Super::OnRegister();
	RefreshPreviewResources();
}

void UResourceCarryComponent::OnUnregister()
{
	if (PreviewResourcesMesh)
	{
		PreviewResourcesMesh->DestroyComponent();
		PreviewResourcesMesh = nullptr;
	}

	PreviewResourceMaterialInstance = nullptr;
	AppliedPreviewResourceMaterial = nullptr;
	Super::OnUnregister();
}

void UResourceCarryComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshPreviewResources();
}

#if WITH_EDITOR
void UResourceCarryComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshPreviewResources();
}
#endif

bool UResourceCarryComponent::IsFull() const
{
	return CurrentOreCount >= MaxOreCount;
}

bool UResourceCarryComponent::CanAddOre(int32 Amount) const
{
	return Amount > 0 && CurrentOreCount < MaxOreCount;
}

int32 UResourceCarryComponent::AddOre(int32 Amount)
{
	if (!CanAddOre(Amount))
	{
		return 0;
	}

	const int32 OldCount = CurrentOreCount;
	const int32 AddAmount = FMath::Min(Amount, MaxOreCount - CurrentOreCount);
	CurrentOreCount += AddAmount;

	if (CurrentOreCount != OldCount)
	{
		BroadcastCarryChanged();
	}

	return AddAmount;
}

int32 UResourceCarryComponent::TakeAllOre()
{
	const int32 TakenAmount = CurrentOreCount;
	if (TakenAmount <= 0)
	{
		return 0;
	}

	CurrentOreCount = 0;
	BroadcastCarryChanged();
	return TakenAmount;
}

void UResourceCarryComponent::ClearOre()
{
	if (CurrentOreCount <= 0)
	{
		RefreshPreviewResources();
		return;
	}

	CurrentOreCount = 0;
	BroadcastCarryChanged();
}

void UResourceCarryComponent::BroadcastCarryChanged()
{
	RefreshPreviewResources();
	OnCarryChanged.Broadcast(CurrentOreCount, MaxOreCount);
}

void UResourceCarryComponent::RefreshPreviewResources()
{
	if (!ConfigurePreviewResourcesMesh())
	{
		if (PreviewResourcesMesh)
		{
			PreviewResourcesMesh->ClearInstances();
			PreviewResourcesMesh->SetVisibility(false, true);
		}
		return;
	}

	PreviewResourcesMesh->SetVisibility(true, true);
	PreviewResourcesMesh->SetHiddenInGame(false);
	PreviewResourcesMesh->ClearInstances();

	const int32 PreviewSlotCount = PreviewResourceTransforms.Num();
	UWorld* World = GetWorld();
	const int32 DisplayCapacity = FMath::Min(MaxOreCount, PreviewSlotCount);
	if (DisplayCapacity <= 0)
	{
		return;
	}

	int32 VisibleInstanceCount = FMath::Clamp(CurrentOreCount, 0, DisplayCapacity);

#if WITH_EDITOR
	const bool bIsBlueprintPreview =
		World && World->WorldType == EWorldType::EditorPreview;
	if (bIsBlueprintPreview && bShowFullPreviewInEditor)
	{
		VisibleInstanceCount = DisplayCapacity;
	}
#endif

	for (int32 Index = 0; Index < VisibleInstanceCount; ++Index)
	{
		PreviewResourcesMesh->AddInstance(PreviewResourceTransforms[Index]);
	}
}

bool UResourceCarryComponent::ConfigurePreviewResourcesMesh()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	USkeletalMeshComponent* OwnerMesh = FindOwnerSkeletalMesh();
	if (!World
		|| !Owner
		|| !PreviewResourceMesh
		|| PreviewSocketName.IsNone()
		|| PreviewResourceTransforms.IsEmpty()
		|| !OwnerMesh
		|| !OwnerMesh->DoesSocketExist(PreviewSocketName))
	{
		return false;
	}

	if (!PreviewResourcesMesh)
	{
		PreviewResourcesMesh = NewObject<UInstancedStaticMeshComponent>(
			Owner,
			NAME_None,
			RF_Transient
		);
		if (!PreviewResourcesMesh)
		{
			return false;
		}

		PreviewResourcesMesh->CreationMethod = EComponentCreationMethod::Instance;
		PreviewResourcesMesh->SetMobility(EComponentMobility::Movable);
		PreviewResourcesMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewResourcesMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		PreviewResourcesMesh->SetGenerateOverlapEvents(false);
		PreviewResourcesMesh->SetCanEverAffectNavigation(false);
		PreviewResourcesMesh->SetSimulatePhysics(false);
		PreviewResourcesMesh->SetEnableGravity(false);
		PreviewResourcesMesh->SetIsReplicated(false);
		Owner->AddInstanceComponent(PreviewResourcesMesh);
		PreviewResourcesMesh->RegisterComponent();
	}

	if (PreviewResourcesMesh->GetAttachParent() != OwnerMesh
		|| PreviewResourcesMesh->GetAttachSocketName() != PreviewSocketName)
	{
		PreviewResourcesMesh->AttachToComponent(
			OwnerMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			PreviewSocketName
		);
	}

	if (PreviewResourcesMesh->GetStaticMesh() != PreviewResourceMesh)
	{
		PreviewResourcesMesh->SetStaticMesh(PreviewResourceMesh);
		PreviewResourcesMesh->SetMaterial(0, PreviewResourceMesh->GetMaterial(0));
		PreviewResourceMaterialInstance = nullptr;
		AppliedPreviewResourceMaterial = nullptr;
	}

	if (PreviewResourceMaterial)
	{
		if (!PreviewResourceMaterialInstance
			|| AppliedPreviewResourceMaterial != PreviewResourceMaterial)
		{
			PreviewResourceMaterialInstance = UMaterialInstanceDynamic::Create(
				PreviewResourceMaterial,
				this
			);
			PreviewResourcesMesh->SetMaterial(0, PreviewResourceMaterialInstance);
			AppliedPreviewResourceMaterial = PreviewResourceMaterial;
		}
	}
	else if (PreviewResourceMaterialInstance || AppliedPreviewResourceMaterial)
	{
		PreviewResourcesMesh->SetMaterial(0, PreviewResourceMesh->GetMaterial(0));
		PreviewResourceMaterialInstance = nullptr;
		AppliedPreviewResourceMaterial = nullptr;
	}

	return true;
}

USkeletalMeshComponent* UResourceCarryComponent::FindOwnerSkeletalMesh() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (const ACharacter* Character = Cast<ACharacter>(Owner))
	{
		if (USkeletalMeshComponent* CharacterMesh = Character->GetMesh())
		{
			return CharacterMesh;
		}
	}

	return Owner->FindComponentByClass<USkeletalMeshComponent>();
}
