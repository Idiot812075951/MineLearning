#include "WarehouseDepot.h"

#include "MineLearning/AI/HaulerCharacter.h"
#include "ItemPickup.h"
#include "ResourceStorageComponent.h"
#include "ItemTypes.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace WarehouseTemporaryCoinDispatch
{
	constexpr int32 CoinBatchSize = 2;
	const FName RobotCenterStorageTag(TEXT("RobotCenter.CoinStorage"));

	int32 GetDispatchableCoinCount(int32 StoredCoinCount)
	{
		return FMath::Max(StoredCoinCount, 0) / CoinBatchSize * CoinBatchSize;
	}
}

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
	DispatchCoinBatchesToRobotCenterForTesting();
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

int32 AWarehouseDepot::DispatchCoinBatchesToRobotCenterForTesting()
{
	if (bTemporaryCoinDispatchInProgress || !HasAuthority())
	{
		return 0;
	}

	UResourceStorageComponent* WarehouseStorage = GetStorageComponent();
	UResourceStorageComponent* RobotCenterStorage = nullptr;
	USceneComponent* PaymentDropPoint = nullptr;
	USceneComponent* WarehouseDockPoint = FindSceneComponent(TEXT("P_Warehouse_DockPoint"));
	if (!WarehouseStorage
		|| !WarehouseDockPoint
		|| !FindTemporaryRobotCenterDeliveryTarget(RobotCenterStorage, PaymentDropPoint))
	{
		return 0;
	}

	TGuardValue<bool> DispatchGuard(bTemporaryCoinDispatchInProgress, true);
	FItemStack CoinBatch;
	CoinBatch.ItemType = EItemType::Coin;
	CoinBatch.Amount = WarehouseTemporaryCoinDispatch::CoinBatchSize;

	int32 DispatchedCoinCount = 0;
	while (WarehouseStorage->GetStoredItemAmount(EItemType::Coin) >= CoinBatch.Amount
		&& RobotCenterStorage->CanAddItem(CoinBatch))
	{
		FTransform SpawnTransform = WarehouseDockPoint->GetComponentTransform();
		SpawnTransform.SetLocation(ResolveOutboundPickupLocation(WarehouseDockPoint));
		SpawnTransform.SetScale3D(FVector::OneVector);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AItemPickup* OutboundCoin = GetWorld()->SpawnActor<AItemPickup>(
			AItemPickup::StaticClass(),
			SpawnTransform,
			SpawnParameters);
		if (!OutboundCoin)
		{
			break;
		}

		TArray<TObjectPtr<UStaticMesh>> CoinMeshes;
		if (UStaticMesh* CoinMesh = InventoryCoinVisual->GetStaticMesh())
		{
			CoinMeshes.Add(CoinMesh);
		}
		OutboundCoin->InitializeItem(
			CoinBatch,
			CoinMeshes,
			MineLearningItemVisual::GoldCoinScale);
		OutboundCoin->ReleaseStationaryForCollection();
		OutboundCoin->SetExplicitDeliveryTarget(
			RobotCenterStorage->GetOwner(),
			RobotCenterStorage,
			PaymentDropPoint);

		if (!WarehouseStorage->RemoveItem(CoinBatch))
		{
			OutboundCoin->Destroy();
			break;
		}

		DispatchedCoinCount += CoinBatch.Amount;
	}

	if (DispatchedCoinCount > 0)
	{
		// TODO: Replace this temporary auto-dispatch with a real Warehouse
		// outbound-order queue. The Hauler movement and PaymentDropPoint delivery
		// performed after this point are real, not an inventory teleport.
		UE_LOG(LogTemp, Display,
			TEXT("[Warehouse][TEMP] Dispatched %d Coin for Hauler delivery to PaymentDropPoint"),
			DispatchedCoinCount);
	}

	return DispatchedCoinCount;
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
	DispatchCoinBatchesToRobotCenterForTesting();
}

bool AWarehouseDepot::FindTemporaryRobotCenterDeliveryTarget(
	UResourceStorageComponent*& OutStorage,
	USceneComponent*& OutDropPoint) const
{
	OutStorage = nullptr;
	OutDropPoint = nullptr;
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// TODO: Replace the component tag/name lookup with a formal outbound-order
	// destination descriptor once the logistics system owns second-leg routes.
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		TInlineComponentArray<UResourceStorageComponent*> StorageComponents(*ActorIterator);
		for (UResourceStorageComponent* CandidateStorage : StorageComponents)
		{
			if (CandidateStorage
				&& CandidateStorage != GetStorageComponent()
				&& CandidateStorage->ComponentHasTag(
					WarehouseTemporaryCoinDispatch::RobotCenterStorageTag))
			{
				TInlineComponentArray<USceneComponent*> SceneComponents(*ActorIterator);
				for (USceneComponent* CandidatePoint : SceneComponents)
				{
					if (CandidatePoint && CandidatePoint->GetFName() == TEXT("PaymentDropPoint"))
					{
						OutStorage = CandidateStorage;
						OutDropPoint = CandidatePoint;
						return true;
					}
				}
			}
		}
	}

	return false;
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
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WarehouseOutboundCoinGround), false, this);
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

	// TODO: Remove this fallback with the temporary Warehouse outbound hook.
	// The actor origin is authored on the work floor, so this remains grounded
	// if a map intentionally has no WorldStatic surface below the dock probe.
	return FVector(ProbeLocation.X, ProbeLocation.Y, GetActorLocation().Z + 5.0f);
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

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWarehouseTemporaryCoinDispatchTest,
	"MineLearning.Warehouse.TemporaryCoinDispatch.TwoCoinBatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWarehouseTemporaryCoinDispatchTest::RunTest(const FString& Parameters)
{
	auto VerifyDispatchAmount = [this](
		const TCHAR* CaseName,
		int32 InitialCoinCount,
		int32 ExpectedDispatchableCoinCount)
	{
		TestEqual(
			CaseName,
			WarehouseTemporaryCoinDispatch::GetDispatchableCoinCount(InitialCoinCount),
			ExpectedDispatchableCoinCount);
	};

	VerifyDispatchAmount(TEXT("One Coin"), 1, 0);
	VerifyDispatchAmount(TEXT("Two Coins"), 2, 2);
	VerifyDispatchAmount(TEXT("Four Coins"), 4, 4);
	return true;
}
#endif
