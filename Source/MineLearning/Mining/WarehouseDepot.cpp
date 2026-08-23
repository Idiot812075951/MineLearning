#include "WarehouseDepot.h"

#include "MineLearning/AI/HaulerCharacter.h"
#include "ResourceStorageComponent.h"
#include "ItemTypes.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AWarehouseDepot::AWarehouseDepot()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	WorkerInteractionTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("WarehouseWorkerTrigger"));
	WorkerInteractionTrigger->SetupAttachment(GetRootComponent());
	WorkerInteractionTrigger->SetBoxExtent(FVector(135.0f, 150.0f, 125.0f));
	WorkerInteractionTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WorkerInteractionTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	WorkerInteractionTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WorkerInteractionTrigger->SetGenerateOverlapEvents(true);

	InventoryCoinVisual = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WarehouseInventoryCoins"));
	InventoryCoinVisual->SetupAttachment(GetRootComponent());
	InventoryCoinVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InventoryCoinVisual->SetGenerateOverlapEvents(false);
	InventoryCoinVisual->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GoldCoinMesh(
		TEXT("/Game/MineLearning/Mining/Resources/Coin/SM_GoldCoin.SM_GoldCoin"));
	if (GoldCoinMesh.Succeeded())
	{
		InventoryCoinVisual->SetStaticMesh(GoldCoinMesh.Object);
	}
}

void AWarehouseDepot::BeginPlay()
{
	Super::BeginPlay();

	CacheAuthoredComponents();
	WorkerInteractionTrigger->OnComponentBeginOverlap.AddDynamic(
		this, &AWarehouseDepot::HandleWorkerEnter);
	WorkerInteractionTrigger->OnComponentEndOverlap.AddDynamic(
		this, &AWarehouseDepot::HandleWorkerExit);

	if (UResourceStorageComponent* Storage = GetStorageComponent())
	{
		Storage->OnStorageChanged.AddDynamic(this, &AWarehouseDepot::HandleStorageChanged);
	}

	bDoorOpenRequested = false;
	CurrentDoorRoll = 0.0f;
	if (DoorPivotComponent)
	{
		FRotator Rotation = DoorPivotComponent->GetRelativeRotation();
		Rotation.Roll = 0.0f;
		DoorPivotComponent->SetRelativeRotation(Rotation);
	}
	UpdateDoorFeedback(false);
	RefreshInventoryVisual();
}

void AWarehouseDepot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!DoorPivotComponent)
	{
		return;
	}

	const float TargetRoll = bDoorOpenRequested ? DoorOpenRoll : 0.0f;
	CurrentDoorRoll = FMath::FInterpConstantTo(
		CurrentDoorRoll,
		TargetRoll,
		DeltaSeconds,
		DoorRotationSpeed);

	FRotator Rotation = DoorPivotComponent->GetRelativeRotation();
	Rotation.Roll = CurrentDoorRoll;
	DoorPivotComponent->SetRelativeRotation(Rotation);

	const bool bFullyOpen = bDoorOpenRequested
		&& FMath::IsNearlyEqual(CurrentDoorRoll, DoorOpenRoll, 0.25f);
	UpdateDoorFeedback(bFullyOpen);
}

void AWarehouseDepot::OpenWarehouse()
{
	bDoorOpenRequested = true;
}

void AWarehouseDepot::CloseWarehouse()
{
	bDoorOpenRequested = false;
	UpdateDoorFeedback(false);
}

bool AWarehouseDepot::AcceptItem_Implementation(const FItemStack& Item)
{
	if (!Super::AcceptItem_Implementation(Item))
	{
		return false;
	}

	OpenWarehouse();
	RefreshInventoryVisual();
	GetWorldTimerManager().SetTimer(
		CloseDoorTimer,
		this,
		&AWarehouseDepot::TryCloseAfterDelivery,
		1.0f,
		false);
	return true;
}

void AWarehouseDepot::HandleWorkerEnter(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (AHaulerCharacter* Worker = Cast<AHaulerCharacter>(OtherActor))
	{
		ActiveWorker = Worker;
		OpenWarehouse();
	}
}

void AWarehouseDepot::HandleWorkerExit(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	if (OtherActor == ActiveWorker)
	{
		ActiveWorker = nullptr;
		CloseWarehouse();
	}
}

void AWarehouseDepot::HandleStorageChanged(int32 StoredOreCount)
{
	RefreshInventoryVisual();
}

void AWarehouseDepot::CacheAuthoredComponents()
{
	DoorPivotComponent = FindSceneComponent(TEXT("DoorPivot"));
	BarrierVisualComponent = Cast<UPrimitiveComponent>(FindSceneComponent(TEXT("BarrierVisual")));
	DoorSafetyBlockerComponent = Cast<UPrimitiveComponent>(
		FindSceneComponent(TEXT("BC_Warehouse_DoorSafetyBlocker")));

	if (USceneComponent* DockPoint = FindSceneComponent(TEXT("P_Warehouse_DockPoint")))
	{
		WorkerInteractionTrigger->AttachToComponent(
			DockPoint,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		WorkerInteractionTrigger->SetRelativeLocation(FVector(0.0f, -90.0f, -35.0f));
	}

	if (USceneComponent* CargoInside = FindSceneComponent(TEXT("P_Warehouse_CargoInside")))
	{
		InventoryCoinVisual->AttachToComponent(
			CargoInside,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

void AWarehouseDepot::RefreshInventoryVisual()
{
	if (!InventoryCoinVisual)
	{
		return;
	}

	InventoryCoinVisual->ClearInstances();
	const UResourceStorageComponent* Storage = GetStorageComponent();
	const int32 CoinCount = Storage
		? FMath::Min(Storage->GetStoredItemAmount(EItemType::Coin), MaxVisibleCoins)
		: 0;

	for (int32 Index = 0; Index < CoinCount; ++Index)
	{
		const int32 Column = Index % 3;
		const int32 Row = (Index / 3) % 2;
		const int32 Layer = Index / 6;
		const FVector Location(
			(Column - 1) * 38.0f,
			(Row - 0.5f) * 40.0f,
			Layer * 9.0f);
		const FRotator Rotation(0.0f, Index * 17.0f, 0.0f);
		InventoryCoinVisual->AddInstance(
			FTransform(Rotation, Location, FVector(MineLearningItemVisual::GoldCoinScale)));
	}

	InventoryCoinVisual->SetVisibility(CoinCount > 0, true);
}

void AWarehouseDepot::UpdateDoorFeedback(bool bFullyOpen)
{
	if (BarrierVisualComponent)
	{
		BarrierVisualComponent->SetVisibility(!bFullyOpen, true);
		BarrierVisualComponent->SetHiddenInGame(bFullyOpen);
	}

	if (DoorSafetyBlockerComponent)
	{
		DoorSafetyBlockerComponent->SetCollisionEnabled(
			bFullyOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
}

void AWarehouseDepot::TryCloseAfterDelivery()
{
	if (ActiveWorker && WorkerInteractionTrigger->IsOverlappingActor(ActiveWorker))
	{
		GetWorldTimerManager().SetTimer(
			CloseDoorTimer,
			this,
			&AWarehouseDepot::TryCloseAfterDelivery,
			0.5f,
			false);
		return;
	}

	CloseWarehouse();
}

USceneComponent* AWarehouseDepot::FindSceneComponent(FName ComponentName) const
{
	TInlineComponentArray<USceneComponent*> SceneComponents(this);
	for (USceneComponent* Component : SceneComponents)
	{
		if (Component && Component->GetFName() == ComponentName)
		{
			return Component;
		}
	}
	return nullptr;
}
