#include "ResourceCarryComponent.h"

#include "ItemPickup.h"
#include "ItemLogisticsLibrary.h"
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
	Capacity = FMath::Max(Capacity, 1);
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
	Capacity = FMath::Max(Capacity, 1);
	RefreshPreviewResources();
}
#endif

bool UResourceCarryComponent::IsFull() const
{
	return CurrentItem.Amount >= Capacity;
}

bool UResourceCarryComponent::CanAcceptItem(const FItemStack& Item) const
{
	if (!Item.IsValid() || IsFull())
	{
		return false;
	}

	if (CurrentItem.IsValid() && CurrentItem.ItemType != Item.ItemType)
	{
		return false;
	}

	const EItemCategory Category = UItemLogisticsLibrary::GetItemCategory(Item.ItemType);
	return bAcceptAllCategories || AllowedCategories.Contains(Category);
}

int32 UResourceCarryComponent::AddItem(const FItemStack& Item)
{
	if (!CanAcceptItem(Item))
	{
		return 0;
	}

	const int32 OldCount = CurrentItem.Amount;
	const int32 AddedAmount = FMath::Min(Item.Amount, Capacity - OldCount);
	if (!CurrentItem.IsValid())
	{
		CurrentItem.ItemType = Item.ItemType;
	}
	CurrentItem.Amount = OldCount + AddedAmount;

	if (AddedAmount > 0)
	{
		BroadcastCarryChanged();
	}
	return AddedAmount;
}

int32 UResourceCarryComponent::AddItemWithVisual(
	const FItemStack& Item,
	UStaticMesh* InResourceMesh,
	float InResourceMeshScale)
{
	if (CanAcceptItem(Item) && !CurrentItem.IsValid() && IsValid(InResourceMesh))
	{
		CarriedResourceMesh = InResourceMesh;
		CarriedResourceMeshScale = FMath::Max(InResourceMeshScale, 0.01f);
	}
	return AddItem(Item);
}

FItemStack UResourceCarryComponent::TakeAllItems()
{
	const FItemStack TakenItem = CurrentItem;
	if (!TakenItem.IsValid())
	{
		return TakenItem;
	}

	CurrentItem.Amount = 0;
	CarriedResourceMesh = nullptr;
	CarriedResourceMeshScale = 1.0f;
	BroadcastCarryChanged();
	return TakenItem;
}

void UResourceCarryComponent::ClearItems()
{
	if (!CurrentItem.IsValid())
	{
		CarriedResourceMesh = nullptr;
		CarriedResourceMeshScale = 1.0f;
		RefreshPreviewResources();
		return;
	}

	CurrentItem.Amount = 0;
	CarriedResourceMesh = nullptr;
	CarriedResourceMeshScale = 1.0f;
	BroadcastCarryChanged();
}

int32 UResourceCarryComponent::DropAllItems(const FVector& DropOrigin)
{
	UWorld* World = GetWorld();
	UStaticMesh* DropMesh = GetPreviewResourceMesh();
	const FItemStack CarriedItem = CurrentItem;
	if (!World || !CarriedItem.IsValid() || !IsValid(DropMesh))
	{
		return 0;
	}

	TArray<TObjectPtr<UStaticMesh>> DropMeshes;
	DropMeshes.Add(DropMesh);
	TArray<AItemPickup*> SpawnedPickups;
	SpawnedPickups.Reserve(CarriedItem.Amount);

	for (int32 Index = 0; Index < CarriedItem.Amount; ++Index)
	{
		const FVector SpawnLocation = DropOrigin + FVector(
			FMath::RandRange(-35.0f, 35.0f),
			FMath::RandRange(-35.0f, 35.0f),
			20.0f + Index * 8.0f);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AItemPickup* Pickup = World->SpawnActor<AItemPickup>(
			AItemPickup::StaticClass(),
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!Pickup)
		{
			for (AItemPickup* SpawnedPickup : SpawnedPickups)
			{
				SpawnedPickup->Destroy();
			}
			return 0;
		}

		FItemStack UnitItem;
		UnitItem.ItemType = CarriedItem.ItemType;
		UnitItem.Amount = 1;
		Pickup->InitializeItem(UnitItem, DropMeshes, CarriedResourceMeshScale);
		SpawnedPickups.Add(Pickup);
	}

	ClearItems();
	return SpawnedPickups.Num();
}

void UResourceCarryComponent::ConfigureAcceptance(
	int32 InCapacity,
	bool bInAcceptAllCategories,
	const TArray<EItemCategory>& InAllowedCategories)
{
	Capacity = FMath::Max(InCapacity, 1);
	bAcceptAllCategories = bInAcceptAllCategories;
	AllowedCategories = InAllowedCategories;
	RefreshPreviewResources();
}

int32 UResourceCarryComponent::GetCurrentOreCount() const
{
	return CurrentItem.IsValid() && CurrentItem.ItemType == EItemType::IronOre
		? CurrentItem.Amount
		: 0;
}

bool UResourceCarryComponent::CanAddOre(int32 Amount) const
{
	return CanAcceptItem({EItemType::IronOre, Amount});
}

int32 UResourceCarryComponent::AddOre(int32 Amount)
{
	return AddItem({EItemType::IronOre, Amount});
}

int32 UResourceCarryComponent::AddOreWithVisual(int32 Amount, UStaticMesh* InResourceMesh)
{
	return AddItemWithVisual({EItemType::IronOre, Amount}, InResourceMesh);
}

int32 UResourceCarryComponent::TakeAllOre()
{
	if (GetCurrentOreCount() <= 0)
	{
		return 0;
	}
	return TakeAllItems().Amount;
}

void UResourceCarryComponent::BroadcastCarryChanged()
{
	RefreshPreviewResources();
	OnCarryChanged.Broadcast(CurrentItem.Amount, Capacity);
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
	const int32 DisplayCapacity = FMath::Min(Capacity, PreviewSlotCount);
	if (DisplayCapacity <= 0)
	{
		return;
	}

	int32 VisibleInstanceCount = FMath::Clamp(CurrentItem.Amount, 0, DisplayCapacity);

#if WITH_EDITOR
	const bool bIsBlueprintPreview = World && World->WorldType == EWorldType::EditorPreview;
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
	UStaticMesh* ResourceMesh = GetPreviewResourceMesh();
	if (!World || !Owner || !ResourceMesh || PreviewSocketName.IsNone()
		|| PreviewResourceTransforms.IsEmpty() || !OwnerMesh
		|| !OwnerMesh->DoesSocketExist(PreviewSocketName))
	{
		return false;
	}

	if (!PreviewResourcesMesh)
	{
		PreviewResourcesMesh = NewObject<UInstancedStaticMeshComponent>(Owner, NAME_None, RF_Transient);
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
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			PreviewSocketName);
	}

	if (PreviewResourcesMesh->GetStaticMesh() != ResourceMesh)
	{
		PreviewResourcesMesh->SetStaticMesh(ResourceMesh);
		PreviewResourcesMesh->SetMaterial(0, ResourceMesh->GetMaterial(0));
		PreviewResourceMaterialInstance = nullptr;
		AppliedPreviewResourceMaterial = nullptr;
	}

	if (PreviewResourceMaterial)
	{
		if (!PreviewResourceMaterialInstance || AppliedPreviewResourceMaterial != PreviewResourceMaterial)
		{
			PreviewResourceMaterialInstance = UMaterialInstanceDynamic::Create(PreviewResourceMaterial, this);
			PreviewResourcesMesh->SetMaterial(0, PreviewResourceMaterialInstance);
			AppliedPreviewResourceMaterial = PreviewResourceMaterial;
		}
	}
	else if (PreviewResourceMaterialInstance || AppliedPreviewResourceMaterial)
	{
		PreviewResourcesMesh->SetMaterial(0, ResourceMesh->GetMaterial(0));
		PreviewResourceMaterialInstance = nullptr;
		AppliedPreviewResourceMaterial = nullptr;
	}

	return true;
}

UStaticMesh* UResourceCarryComponent::GetPreviewResourceMesh() const
{
	return IsValid(CarriedResourceMesh) ? CarriedResourceMesh.Get() : PreviewResourceMesh;
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
