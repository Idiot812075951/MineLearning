#include "WarehouseDepot.h"

#include "MineLearning/AI/HaulerCharacter.h"
#include "ItemLogisticsLibrary.h"
#include "ItemPickup.h"
#include "ItemRules.h"
#include "ItemTypes.h"
#include "OreProcessorMachine.h"
#include "ResourceStorageComponent.h"
#include "SellStation.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
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
	WorkerInteractionTrigger->SetCanEverAffectNavigation(false);

	InventoryCoinVisual = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WarehouseInventoryCoins"));
	InventoryCoinVisual->SetupAttachment(GetRootComponent());
	InventoryCoinVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InventoryCoinVisual->SetGenerateOverlapEvents(false);
	InventoryCoinVisual->SetCanEverAffectNavigation(false);
	InventoryCoinVisual->SetMobility(EComponentMobility::Movable);

	InventoryOreVisual = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WarehouseInventoryOre"));
	InventoryOreVisual->SetupAttachment(GetRootComponent());
	InventoryOreVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InventoryOreVisual->SetGenerateOverlapEvents(false);
	InventoryOreVisual->SetCanEverAffectNavigation(false);
	InventoryOreVisual->SetMobility(EComponentMobility::Movable);

	InventoryIngotVisual = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WarehouseInventoryIngots"));
	InventoryIngotVisual->SetupAttachment(GetRootComponent());
	InventoryIngotVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InventoryIngotVisual->SetGenerateOverlapEvents(false);
	InventoryIngotVisual->SetCanEverAffectNavigation(false);
	InventoryIngotVisual->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GoldCoinMesh(
		TEXT("/Game/MineLearning/Mining/Resources/Coin/SM_GoldCoin.SM_GoldCoin"));
	if (GoldCoinMesh.Succeeded())
	{
		InventoryCoinVisual->SetStaticMesh(GoldCoinMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> IronOreMesh(
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Meshes/SM_Ore_Iron_Drop_01.SM_Ore_Iron_Drop_01"));
	if (IronOreMesh.Succeeded())
	{
		OutboundOreMesh = IronOreMesh.Object;
		InventoryOreVisual->SetStaticMesh(IronOreMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> IronIngotMeshFinder(
		TEXT("/Game/MineLearning/Mining/Resources/IronIngot/SM_IronIngot.SM_IronIngot"));
	if (IronIngotMeshFinder.Succeeded())
	{
		IronIngotMesh = IronIngotMeshFinder.Object;
		InventoryIngotVisual->SetStaticMesh(IronIngotMeshFinder.Object);
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

void AWarehouseDepot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UResourceStorageComponent* Storage = GetStorageComponent())
	{
		Storage->OnStorageChanged.RemoveDynamic(this, &AWarehouseDepot::HandleStorageChanged);
	}

	for (AItemPickup* Pickup : PendingOrderPickups)
	{
		if (IsValid(Pickup))
		{
			Pickup->Destroy();
		}
	}
	PendingOrderPickups.Reset();
	Super::EndPlay(EndPlayReason);
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

bool AWarehouseDepot::IsPlayerInInteractionRange(const APawn* PlayerPawn) const
{
	if (!IsValid(PlayerPawn))
	{
		return false;
	}

	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	if (WorkerInteractionTrigger)
	{
		const FBox TriggerBounds = WorkerInteractionTrigger->Bounds.GetBox();
		const FBox2D TriggerFootprint(
			FVector2D(TriggerBounds.Min),
			FVector2D(TriggerBounds.Max));
		return TriggerFootprint.ComputeSquaredDistanceToPoint(FVector2D(PlayerLocation))
			<= FMath::Square(PlayerInteractionRange);
	}

	return FVector::DistSquared2D(PlayerLocation, GetActorLocation())
		<= FMath::Square(PlayerInteractionRange);
}

TArray<FWarehouseItemViewData> AWarehouseDepot::GetInventoryViewData() const
{
	TArray<FWarehouseItemViewData> Result;
	const UResourceStorageComponent* Storage = GetStorageComponent();
	if (!Storage)
	{
		return Result;
	}

	static const EItemType SupportedItems[] =
	{
		EItemType::IronOre,
		EItemType::IronIngot,
		EItemType::Coin,
		EItemType::Ammo
	};
	Result.Reserve(UE_ARRAY_COUNT(SupportedItems));
	for (EItemType ItemType : SupportedItems)
	{
		FItemStack LookupItem;
		LookupItem.ItemType = ItemType;
		LookupItem.Amount = 1;

		FItemRuleRow Rule;
		UItemLogisticsLibrary::GetItemRule(LookupItem, Rule);

		FWarehouseItemViewData& ViewData = Result.AddDefaulted_GetRef();
		ViewData.ItemType = ItemType;
		ViewData.DisplayName = Rule.DisplayName.IsEmpty()
			? StaticEnum<EItemType>()->GetDisplayNameTextByValue(static_cast<int64>(ItemType))
			: Rule.DisplayName;
		ViewData.Icon = Rule.Icon;
		ViewData.TotalAmount = Storage->GetStoredItemAmount(ItemType);
		ViewData.AvailableAmount = Storage->GetAvailableItemAmount(ItemType);
		ViewData.FrozenAmount = Storage->GetReservedItemAmount(ItemType);
		ViewData.UnitSellPrice = FMath::Max(Rule.UnitSellPrice, 0);

		AActor* ActiveDestination = nullptr;
		for (const AItemPickup* Pickup : PendingOrderPickups)
		{
			if (IsValid(Pickup)
				&& Pickup->GetItemStack().ItemType == ItemType
				&& Pickup->HasUsableExplicitDeliveryTarget())
			{
				ActiveDestination = Pickup->GetExplicitDeliveryActor();
				break;
			}
		}

		ASellStation* SellDestination = ViewData.UnitSellPrice > 0
			? FindSellStation(LookupItem)
			: nullptr;
		AOreProcessorMachine* ProcessDestination = ItemType == EItemType::IronOre
			? FindProcessor()
			: nullptr;
		if (const ASellStation* ActiveSellStation = Cast<ASellStation>(ActiveDestination))
		{
			ViewData.DeliveryDestination = ActiveSellStation->GetStationDisplayName();
		}
		else if (const AOreProcessorMachine* ActiveProcessor = Cast<AOreProcessorMachine>(ActiveDestination))
		{
			ViewData.DeliveryDestination = ActiveProcessor->GetProcessorDisplayName();
		}
		else if (ProcessDestination && SellDestination)
		{
			ViewData.DeliveryDestination = NSLOCTEXT(
				"MineLearning", "WarehouseIronOreDestinations", "加工机 / 出售点");
		}
		else if (ProcessDestination)
		{
			ViewData.DeliveryDestination = ProcessDestination->GetProcessorDisplayName();
		}
		else if (SellDestination)
		{
			ViewData.DeliveryDestination = SellDestination->GetStationDisplayName();
		}
		ViewData.bCanSell = ViewData.UnitSellPrice > 0
			&& ViewData.AvailableAmount > 0
			&& IsValid(SellDestination);
		ViewData.bCanProcess = ItemType == EItemType::IronOre
			&& ViewData.AvailableAmount > 0
			&& IsValid(ProcessDestination);
	}
	return Result;
}

bool AWarehouseDepot::RequestSell(EItemType ItemType, int32 Amount)
{
	if (ItemType != EItemType::IronOre || Amount <= 0
		|| UItemLogisticsLibrary::GetUnitSellPrice(ItemType) <= 0)
	{
		return false;
	}

	FItemStack SaleItem;
	SaleItem.ItemType = ItemType;
	SaleItem.Amount = Amount;
	ASellStation* Destination = FindSellStation(SaleItem);
	return Destination && RequestDeliveryOrder(
		SaleItem,
		Destination,
		Destination->GetRobotApproachPoint());
}

bool AWarehouseDepot::RequestProcess(EItemType ItemType, int32 Amount)

{
	if (ItemType != EItemType::IronOre || Amount <= 0)
	{
		return false;
	}

	AOreProcessorMachine* Destination = FindProcessor();
	FItemStack ProcessItem;
	ProcessItem.ItemType = ItemType;
	ProcessItem.Amount = Amount;
	return Destination && RequestDeliveryOrder(
		ProcessItem,
		Destination,
		Destination->GetDeliveryPointComponent());
}

bool AWarehouseDepot::RequestDeliveryOrder(
	const FItemStack& Item,
	AActor* Destination,
	USceneComponent* DestinationPoint)
{
	UResourceStorageComponent* Storage = GetStorageComponent();
	USceneComponent* DockPoint = FindSceneComponent(TEXT("P_Warehouse_DockPoint"));
	if (!Storage || !DockPoint || !GetWorld() || !IsValid(OutboundOreMesh)
		|| !Item.IsValid() || !IsValid(Destination) || !IsValid(DestinationPoint)
		|| !Destination->GetClass()->ImplementsInterface(UItemReceiver::StaticClass())
		|| !Storage->TryReserveItem(Item))
	{
		return false;
	}

	FTransform SpawnTransform = DockPoint->GetComponentTransform();
	SpawnTransform.SetLocation(ResolveOutboundPickupLocation(DockPoint));
	SpawnTransform.SetScale3D(FVector::OneVector);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<TObjectPtr<UStaticMesh>> OreMeshes;
	OreMeshes.Add(OutboundOreMesh);
	TArray<AItemPickup*> SpawnedOrders;
	SpawnedOrders.Reserve(Item.Amount);
	for (int32 Index = 0; Index < Item.Amount; ++Index)
	{
		AItemPickup* OrderPickup = GetWorld()->SpawnActor<AItemPickup>(
			AItemPickup::StaticClass(),
			SpawnTransform,
			SpawnParameters);
		if (!OrderPickup)
		{
			for (AItemPickup* SpawnedOrder : SpawnedOrders)
			{
				SpawnedOrder->Destroy();
			}
			Storage->ReleaseReservedItem(Item);
			return false;
		}

		FItemStack UnitItem;
		UnitItem.ItemType = Item.ItemType;
		UnitItem.Amount = 1;
		OrderPickup->InitializeItem(UnitItem, OreMeshes);
		OrderPickup->ReleaseStationaryForCollection();
		OrderPickup->SetWaitingVisualEnabled(false);
		OrderPickup->SetRequiresHauler(true);
		OrderPickup->SetExplicitDeliveryTarget(Destination, nullptr, DestinationPoint);
		SpawnedOrders.Add(OrderPickup);
	}

	for (AItemPickup* OrderPickup : SpawnedOrders)
	{
		OrderPickup->SetReservationSource(Storage);
		PendingOrderPickups.Add(OrderPickup);
	}
	BroadcastWarehouseChanged();
	return true;
}

int32 AWarehouseDepot::CancelPendingOrder(EItemType ItemType, int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	CleanupOrderPickups();
	int32 RemainingToCancel = Amount;
	int32 CanceledAmount = 0;
	for (int32 Index = PendingOrderPickups.Num() - 1;
		Index >= 0 && RemainingToCancel > 0;
		--Index)
	{
		AItemPickup* Pickup = PendingOrderPickups[Index];
		if (!IsValid(Pickup) || Pickup->GetItemStack().ItemType != ItemType)
		{
			continue;
		}

		const int32 CanceledFromPickup = Pickup->CancelReservedAmount(RemainingToCancel);
		RemainingToCancel -= CanceledFromPickup;
		CanceledAmount += CanceledFromPickup;
	}
	CleanupOrderPickups();
	if (CanceledAmount > 0)
	{
		BroadcastWarehouseChanged();
	}
	return CanceledAmount;
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
	CleanupOrderPickups();
	RefreshInventoryVisual();
	BroadcastWarehouseChanged();
}

ASellStation* AWarehouseDepot::FindSellStation(const FItemStack& Item) const
{
	UWorld* World = GetWorld();
	if (!World || !Item.IsValid())
	{
		return nullptr;
	}

	ASellStation* NearestStation = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<ASellStation> StationIterator(World); StationIterator; ++StationIterator)
	{
		ASellStation* Station = *StationIterator;
		if (!IsValid(Station)
			|| Station->IsActorBeingDestroyed()
			|| !IItemReceiver::Execute_CanAcceptItem(Station, Item))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(
			GetActorLocation(),
			Station->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestStation = Station;
		}
	}
	return NearestStation;
}

AOreProcessorMachine* AWarehouseDepot::FindProcessor() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AOreProcessorMachine* NearestProcessor = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AOreProcessorMachine> ProcessorIterator(World);
		ProcessorIterator;
		++ProcessorIterator)
	{
		AOreProcessorMachine* Processor = *ProcessorIterator;
		if (!IsValid(Processor)
			|| Processor->IsActorBeingDestroyed()
			|| !IsValid(Processor->GetDeliveryPointComponent()))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(
			GetActorLocation(),
			Processor->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestProcessor = Processor;
		}
	}
	return NearestProcessor;
}

void AWarehouseDepot::CleanupOrderPickups()
{
	PendingOrderPickups.RemoveAll(
		[](const AItemPickup* Pickup)
		{
			return !IsValid(Pickup) || Pickup->IsActorBeingDestroyed();
		});
}

void AWarehouseDepot::BroadcastWarehouseChanged()
{
	OnWarehouseChanged.Broadcast();
}

FVector AWarehouseDepot::ResolveOutboundPickupLocation(const USceneComponent* DockPoint) const
{
	if (!DockPoint)
	{
		return GetActorLocation();
	}

	const FVector ProbeLocation = DockPoint->GetComponentLocation()
		+ DockPoint->GetForwardVector() * 120.0f;
	const FVector TraceStart = ProbeLocation + FVector(0.0f, 0.0f, 200.0f);
	const FVector TraceEnd = ProbeLocation - FVector(0.0f, 0.0f, 500.0f);
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WarehouseOutboundOrderGround), false, this);
	const FCollisionObjectQueryParams ObjectQuery(ECC_WorldStatic);
	if (const UWorld* World = GetWorld();
		World && World->LineTraceSingleByObjectType(
			GroundHit,
			TraceStart,
			TraceEnd,
			ObjectQuery,
			QueryParams))
	{
		return GroundHit.ImpactPoint + FVector(0.0f, 0.0f, 5.0f);
	}

	// The actor origin is authored on the work floor, so orders remain grounded
	// if a map intentionally has no WorldStatic surface below the dock probe.
	return FVector(ProbeLocation.X, ProbeLocation.Y, GetActorLocation().Z + 5.0f);
}

void AWarehouseDepot::CacheAuthoredComponents()
{
	DoorPivotComponent = FindSceneComponent(TEXT("DoorPivot"));
	BarrierVisualComponent = Cast<UPrimitiveComponent>(FindSceneComponent(TEXT("BarrierVisual")));
	DoorSafetyBlockerComponent = Cast<UPrimitiveComponent>(
		FindSceneComponent(TEXT("BC_Warehouse_DoorSafetyBlocker")));

	// The doorway opens at runtime while the project uses a static NavMesh. Keep its
	// changing collision out of navigation so an approaching worker can reach the
	// overlap trigger that opens it.
	if (BarrierVisualComponent)
	{
		BarrierVisualComponent->SetCanEverAffectNavigation(false);
	}
	if (DoorSafetyBlockerComponent)
	{
		DoorSafetyBlockerComponent->SetCanEverAffectNavigation(false);
	}
	if (UPrimitiveComponent* DoorComponent = Cast<UPrimitiveComponent>(FindSceneComponent(TEXT("Door"))))
	{
		DoorComponent->SetCanEverAffectNavigation(false);
	}

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
		InventoryOreVisual->AttachToComponent(
			CargoInside,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		InventoryIngotVisual->AttachToComponent(
			CargoInside,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

void AWarehouseDepot::RefreshInventoryVisual()
{
	if (!InventoryCoinVisual || !InventoryOreVisual || !InventoryIngotVisual)
	{
		return;
	}

	InventoryCoinVisual->ClearInstances();
	InventoryOreVisual->ClearInstances();
	InventoryIngotVisual->ClearInstances();
	const UResourceStorageComponent* Storage = GetStorageComponent();
	const int32 CoinCount = Storage
		? FMath::Min(Storage->GetStoredItemAmount(EItemType::Coin), MaxVisibleItemsPerType)
		: 0;
	const int32 OreCount = Storage
		? FMath::Min(Storage->GetStoredItemAmount(EItemType::IronOre), MaxVisibleItemsPerType)
		: 0;
	const int32 IngotCount = Storage
		? FMath::Min(Storage->GetStoredItemAmount(EItemType::IronIngot), MaxVisibleItemsPerType)
		: 0;

	AddInventoryInstances(InventoryOreVisual, OreCount, -52.0f, 29.0f);
	AddInventoryInstances(InventoryIngotVisual, IngotCount, 0.0f, 11.0f);
	AddInventoryInstances(InventoryCoinVisual, CoinCount, 52.0f, 17.0f);

	InventoryOreVisual->SetVisibility(OreCount > 0, true);
	InventoryIngotVisual->SetVisibility(IngotCount > 0, true);
	InventoryCoinVisual->SetVisibility(CoinCount > 0, true);
}

void AWarehouseDepot::AddInventoryInstances(
	UInstancedStaticMeshComponent* Visual,
	int32 Count,
	float GroupWorldOffsetY,
	float YawStepDegrees)
{
	UStaticMesh* Mesh = Visual ? Visual->GetStaticMesh() : nullptr;
	if (!Visual || !IsValid(Mesh) || Count <= 0)
	{
		return;
	}

	const FVector ParentScale = Visual->GetComponentScale().GetAbs();
	const FVector InstanceScale = MineLearningItemVisual::GetRelativeScale(Mesh, ParentScale);
	const FVector WorldSize = MineLearningItemVisual::GetWorldSize(Mesh);
	const float LocalColumnSpacing = 42.0f / FMath::Max(ParentScale.X, UE_SMALL_NUMBER);
	const float LocalRowSpacing = 40.0f / FMath::Max(ParentScale.Y, UE_SMALL_NUMBER);
	const float LocalGroupOffsetY = GroupWorldOffsetY / FMath::Max(ParentScale.Y, UE_SMALL_NUMBER);
	const float LocalItemHeight = WorldSize.Z / FMath::Max(ParentScale.Z, UE_SMALL_NUMBER);
	const float LocalVerticalGap = 2.0f / FMath::Max(ParentScale.Z, UE_SMALL_NUMBER);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const int32 StackIndex = Index / MineLearningItemVisual::DefaultStackHeight;
		const int32 StackLevel = Index % MineLearningItemVisual::DefaultStackHeight;
		const int32 Column = StackIndex % 4;
		const int32 Row = StackIndex / 4;
		const FVector Location(
			(Column - 1.5f) * LocalColumnSpacing,
			LocalGroupOffsetY + Row * LocalRowSpacing,
			LocalItemHeight * 0.5f + StackLevel * (LocalItemHeight + LocalVerticalGap));
		const FRotator Rotation(0.0f, Index * YawStepDegrees, 0.0f);
		Visual->AddInstance(FTransform(Rotation, Location, InstanceScale));
	}
}

void AWarehouseDepot::UpdateDoorFeedback(bool bFullyOpen)
{
	if (BarrierVisualComponent)
	{
		BarrierVisualComponent->SetVisibility(!bFullyOpen, true);
		BarrierVisualComponent->SetHiddenInGame(bFullyOpen);
		BarrierVisualComponent->SetCollisionEnabled(
			bFullyOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
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
