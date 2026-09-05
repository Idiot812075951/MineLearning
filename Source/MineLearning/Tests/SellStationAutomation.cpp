#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "MineLearning/AI/HaulerCharacter.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Mining/ItemLogisticsLibrary.h"
#include "MineLearning/Mining/ItemTypes.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/SellStation.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace SellStationAutomation
{
	UWorld* FindCurrentEditorWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType == EWorldType::Editor && WorldContext.World())
			{
				return WorldContext.World();
			}
		}

		return nullptr;
	}

	template<typename TComponent>
	TComponent* FindNamedComponent(const AActor* Actor, const FName ComponentName)
	{
		TInlineComponentArray<TComponent*> Components(Actor);
		for (TComponent* Component : Components)
		{
			if (Component && Component->GetFName() == ComponentName)
			{
				return Component;
			}
		}

		return nullptr;
	}

	void DestroyTestWorld(UWorld* GameWorld)
	{
		if (!GameWorld)
		{
			return;
		}

		if (GameWorld->AreActorsInitialized())
		{
			for (AActor* Actor : FActorRange(GameWorld))
			{
				if (Actor)
				{
					Actor->RouteEndPlay(EEndPlayReason::LevelTransition);
				}
			}
		}

		GEngine->ShutdownWorldNetDriver(GameWorld);
		GameWorld->DestroyWorld(true);
		GameWorld->SetPhysicsScene(nullptr);
		GEngine->DestroyWorldContext(GameWorld);
		GameWorld->RemoveFromRoot();
	}

	void TickWorldFor(UWorld* World, float Duration)
	{
		constexpr float TickStep = 1.0f / 30.0f;
		float Remaining = Duration;
		while (Remaining > 0.0f)
		{
			++GFrameCounter;
			const float ThisStep = FMath::Min(Remaining, TickStep);
			World->Tick(ELevelTick::LEVELTICK_All, ThisStep);
			Remaining -= ThisStep;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSellStationPresentationContractTest,
	"MineLearning.SellStation.PresentationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSellStationPresentationContractTest::RunTest(const FString& Parameters)
{
	using namespace SellStationAutomation;

	UWorld* World = FindCurrentEditorWorld();
	if (!TestNotNull(TEXT("Current editor world"), World))
	{
		return false;
	}

	ASellStation* SellStation = nullptr;
	for (TActorIterator<ASellStation> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		SellStation = *ActorIterator;
		break;
	}
	if (!TestNotNull(TEXT("Sell Station"), SellStation))
	{
		return false;
	}

	USceneComponent* ItemPoint = FindNamedComponent<USceneComponent>(SellStation, TEXT("ItemDisplayPoint"));
	USceneComponent* CoinPoint = FindNamedComponent<USceneComponent>(SellStation, TEXT("CoinSpawnPoint"));
	UBoxComponent* Blocker = FindNamedComponent<UBoxComponent>(SellStation, TEXT("StationBlocker"));
	UStaticMeshComponent* PresentationItem =
		FindNamedComponent<UStaticMeshComponent>(SellStation, TEXT("PresentationItem"));
	UStaticMeshComponent* ScanPad =
		FindNamedComponent<UStaticMeshComponent>(SellStation, TEXT("ScanPad"));
	UWidgetComponent* ScreenWidget =
		FindNamedComponent<UWidgetComponent>(SellStation, TEXT("ScreenWidget"));
	if (!TestNotNull(TEXT("ItemDisplayPoint"), ItemPoint)
		|| !TestNotNull(TEXT("CoinSpawnPoint"), CoinPoint)
		|| !TestNotNull(TEXT("StationBlocker"), Blocker)
		|| !TestNotNull(TEXT("PresentationItem"), PresentationItem)
		|| !TestNotNull(TEXT("ScanPad"), ScanPad)
		|| !TestNotNull(TEXT("ScreenWidget"), ScreenWidget))
	{
		return false;
	}

	TestEqual(TEXT("Item presentation point"), ItemPoint->GetRelativeLocation(), FVector(0.0f, 20.0f, 85.0f));
	TestEqual(TEXT("Coin outlet point"), CoinPoint->GetRelativeLocation(), FVector(0.0f, 119.0f, 58.0f));
	TestEqual(TEXT("Simple blocker extent"), Blocker->GetUnscaledBoxExtent(), FVector(135.0f, 100.0f, 45.0f));
	TestEqual(TEXT("Simple blocker collision"), Blocker->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
	TestTrue(TEXT("Simple blocker affects navigation"), Blocker->CanEverAffectNavigation());
	TestNotNull(TEXT("Presentation ore mesh"), PresentationItem->GetStaticMesh().Get());
	TestEqual(TEXT("Presentation item has no collision"), PresentationItem->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Presentation item does not affect navigation"), PresentationItem->CanEverAffectNavigation());
	TestTrue(TEXT("Presentation item waits hidden"), PresentationItem->bHiddenInGame);
	TestNotNull(TEXT("Scan pad mesh"), ScanPad->GetStaticMesh().Get());
	TestEqual(TEXT("Scan pad has no collision"), ScanPad->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestNotNull(TEXT("World screen widget class"), ScreenWidget->GetWidgetClass().Get());
	TestEqual(TEXT("World screen draw width"), ScreenWidget->GetDrawSize().X, 1000.0);
	TestEqual(TEXT("World screen draw height"), ScreenWidget->GetDrawSize().Y, 360.0);
	TestEqual(TEXT("World screen location"), ScreenWidget->GetRelativeLocation(), FVector(0.0f, -76.0f, 170.0f));
	TestEqual(TEXT("World screen collision"), ScreenWidget->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse(TEXT("World screen does not affect navigation"), ScreenWidget->CanEverAffectNavigation());

	FItemStack SoldOre;
	SoldOre.ItemType = EItemType::IronOre;
	SoldOre.Amount = 5;
	TestTrue(TEXT("Sale item stack is valid"), SoldOre.IsValid());
	TestTrue(TEXT("Sale item has a positive configured price"),
		UItemLogisticsLibrary::GetUnitSellPrice(SoldOre.ItemType) > 0);
	TestFalse(TEXT("Station begins outside the busy state"),
		SellStation->ActorHasTag(TEXT("SellPresentation.Active")));
	EItemReceiverType ReceiverType = EItemReceiverType::Processor;
	TestTrue(TEXT("Station exposes a receiver type through the logistics router"),
		UItemLogisticsLibrary::TryGetReceiverType(SellStation, ReceiverType));
	TestEqual(TEXT("Station routes as a sell receiver"), ReceiverType, EItemReceiverType::SellPoint);
	TestTrue(TEXT("Station reports priced ore can be accepted"),
		UItemLogisticsLibrary::CanReceiverAcceptItem(SellStation, SoldOre));

	FWorldContext& GameplayWorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	const FName GameplayWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("SellStationAutomationWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* GameplayWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		GameplayWorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("Isolated gameplay world"), GameplayWorld))
	{
		return false;
	}
	GameplayWorld->AddToRoot();
	GameplayWorldContext.SetCurrentWorld(GameplayWorld);
	GameplayWorld->InitializeActorsForPlay(FURL());

	ASellStation* GameplayStation = GameplayWorld->SpawnActor<ASellStation>(
		SellStation->GetClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Gameplay sell station"), GameplayStation))
	{
		DestroyTestWorld(GameplayWorld);
		return false;
	}
	GameplayWorld->BeginPlay();

	TestTrue(TEXT("Priced ore is accepted in a gameplay world"),
		UItemLogisticsLibrary::DeliverItemToReceiver(GameplayStation, SoldOre));
	UInstancedStaticMeshComponent* SaleItemStack =
		FindNamedComponent<UInstancedStaticMeshComponent>(
			GameplayStation, TEXT("SaleItemStackVisual"));
	UStaticMeshComponent* GameplayPresentationItem =
		FindNamedComponent<UStaticMeshComponent>(
			GameplayStation, TEXT("PresentationItem"));
	if (TestNotNull(TEXT("Sale station creates a physical batch stack"), SaleItemStack))
	{
		TestEqual(TEXT("Four instanced goods plus the primary mesh present five items"),
			SaleItemStack->GetInstanceCount(), 4);
		TestTrue(TEXT("Additional sale goods are visible during scanning"),
			SaleItemStack->IsVisible());
	}
	if (TestNotNull(TEXT("Gameplay primary sale item"), GameplayPresentationItem))
	{
		TestFalse(TEXT("Primary fifth sale item is visible during scanning"),
			GameplayPresentationItem->bHiddenInGame);
	}

	AItemPickup* SpawnedCoin = nullptr;
	UInstancedStaticMeshComponent* SpawnedCoinStack = nullptr;
	int32 SpawnedCoinCount = 0;
	for (TActorIterator<AItemPickup> PickupIterator(GameplayWorld); PickupIterator; ++PickupIterator)
	{
		AItemPickup* Candidate = *PickupIterator;
		if (Candidate->GetItemStack().ItemType == EItemType::Coin)
		{
			++SpawnedCoinCount;
			SpawnedCoin = Candidate;
		}
	}

	TestEqual(TEXT("A sale spawns exactly one real coin pickup"), SpawnedCoinCount, 1);
	if (TestNotNull(TEXT("One real coin pickup is spawned"), SpawnedCoin))
	{
		TestEqual(TEXT("Coin pickup stores the full five-item batch payout"), SpawnedCoin->GetAmount(), 5);
		SpawnedCoinStack = FindNamedComponent<UInstancedStaticMeshComponent>(
			SpawnedCoin, TEXT("ItemStackVisual"));
		if (TestNotNull(TEXT("Coin pickup creates its amount-driven visual stack"), SpawnedCoinStack))
		{
			TestEqual(TEXT("Four visual instances plus the pickup root show five coins"),
				SpawnedCoinStack->GetInstanceCount(), 4);
		}
		TestTrue(TEXT("Coin is locked during presentation"), SpawnedCoin->IsTransportLocked());
		TestTrue(TEXT("Coin is hidden before the eject beat"), SpawnedCoin->IsHidden());
	}
	TestFalse(TEXT("Station rejects re-entry during presentation"),
		UItemLogisticsLibrary::CanReceiverAcceptItem(GameplayStation, SoldOre));

	if (SpawnedCoin)
	{
		TickWorldFor(GameplayWorld, 1.0f);
		TestTrue(TEXT("Coin remains hidden throughout the readable scan"), SpawnedCoin->IsHidden());
		if (SaleItemStack)
		{
			TestEqual(TEXT("All five sale goods remain for the readable scan"),
				SaleItemStack->GetInstanceCount(), 4);
		}

		TickWorldFor(GameplayWorld, 1.35f);
		TestFalse(TEXT("Coin becomes visible at the authored outlet after scanning"), SpawnedCoin->IsHidden());
		TestTrue(TEXT("Coin stays locked for the observation beat"), SpawnedCoin->IsTransportLocked());
		if (SpawnedCoinStack)
		{
			TestTrue(TEXT("All five payout coins are visible during the observation beat"),
				SpawnedCoinStack->IsVisible());
			TestEqual(TEXT("The complete payout stack remains after the pop"),
				SpawnedCoinStack->GetInstanceCount(), 4);
		}
		if (SaleItemStack)
		{
			TestEqual(TEXT("Sale goods clear only when payout appears"),
				SaleItemStack->GetInstanceCount(), 0);
		}
		USceneComponent* GameplayCoinPoint = FindNamedComponent<USceneComponent>(
			GameplayStation, TEXT("CoinSpawnPoint"));
		if (TestNotNull(TEXT("Gameplay coin outlet point"), GameplayCoinPoint))
		{
			TestEqual(
				TEXT("Visible payout remains at the authored outlet"),
				SpawnedCoin->GetActorLocation(),
				GameplayCoinPoint->GetComponentLocation());
			TestTrue(
				TEXT("Payout orientation matches the authored warehouse-style coin orientation"),
				SpawnedCoin->GetActorQuat().Equals(
					GameplayCoinPoint->GetComponentQuat(), KINDA_SMALL_NUMBER));
		}

		TickWorldFor(GameplayWorld, 1.20f);
		TestTrue(TEXT("Coin remains locked long enough to be seen"), SpawnedCoin->IsTransportLocked());

		TickWorldFor(GameplayWorld, 0.70f);
		TestFalse(TEXT("Coin unlocks for the existing collection path"),
			SpawnedCoin->IsTransportLocked());
		TestNull(TEXT("Released coin no longer belongs to the station"), SpawnedCoin->GetOwner());
		if (UStaticMesh* SpawnedCoinMesh = SpawnedCoin->Mesh->GetStaticMesh())
		{
			const FVector DisplaySize = SpawnedCoinMesh->GetBounds().BoxExtent * 2.0f
				* SpawnedCoin->Mesh->GetComponentScale();
			TestTrue(TEXT("Released payout uses the shared readable coin size"),
				FMath::IsNearlyEqual(
					DisplaySize.GetAbsMax(),
					MineLearningItemVisual::CoinMaxDimensionCm,
					0.1f));
		}
		TestFalse(TEXT("Station exits the busy state after payout"),
			GameplayStation->ActorHasTag(TEXT("SellPresentation.Active")));
	}

	AHaulerCharacter* GameplayHauler = GameplayWorld->SpawnActor<AHaulerCharacter>(
		FVector(500.0f, 0.0f, 100.0f),
		FRotator::ZeroRotator);
	if (TestNotNull(TEXT("Gameplay hauler"), GameplayHauler))
	{
		UStaticMesh* OreMesh = LoadObject<UStaticMesh>(
			nullptr,
			TEXT("/Game/MineLearning/Mining/Ores/Iron/Meshes/SM_Ore_Iron_Drop_01.SM_Ore_Iron_Drop_01"));
		UResourceCarryComponent* CarryComponent = GameplayHauler->GetResourceCarryComponent();
		if (TestNotNull(TEXT("Gameplay hauler carry component"), CarryComponent)
			&& TestNotNull(TEXT("Gameplay hauler ore mesh"), OreMesh))
		{
			TestEqual(TEXT("Hauler accepts the complete five-item batch"),
				CarryComponent->AddItemWithVisual(SoldOre, OreMesh), 5);
			GameplayHauler->ShowCarriedItem(OreMesh);
			TestEqual(TEXT("Hauler tray renders five physical item instances"),
				CarryComponent->GetWorldPreviewItemCount(), 5);
			UInstancedStaticMeshComponent* CarriedStack =
				FindNamedComponent<UInstancedStaticMeshComponent>(
					GameplayHauler, TEXT("CarriedItemStackVisual"));
			if (TestNotNull(TEXT("Hauler generated tray stack"), CarriedStack))
			{
				TestEqual(TEXT("Hauler stack uses the authored cargo socket"),
					CarriedStack->GetAttachSocketName(), FName(TEXT("S_Cargo")));
			}
			TestTrue(TEXT("Hauler tray is visible with its batch"),
				GameplayHauler->HasVisibleCargo());
		}

		UClass* WarehouseClass = LoadClass<AWarehouseDepot>(
			nullptr,
			TEXT("/Game/MineLearning/Mining/Storage/Blueprints/BP_Warehouse.BP_Warehouse_C"));
		AWarehouseDepot* GameplayWarehouse = WarehouseClass
			? GameplayWorld->SpawnActor<AWarehouseDepot>(
				WarehouseClass,
				FVector(1000.0f, 0.0f, 0.0f),
				FRotator::ZeroRotator)
			: nullptr;
		if (TestNotNull(TEXT("Gameplay warehouse"), GameplayWarehouse))
		{
			TestFalse(TEXT("Warehouse begins without an access request"),
				GameplayWarehouse->IsDoorOpenRequested());
			GameplayWarehouse->BeginWorkerAccess(GameplayHauler);
			TestTrue(TEXT("A worker access session requests the warehouse door open"),
				GameplayWarehouse->IsDoorOpenRequested());
			GameplayWarehouse->EndWorkerAccess(GameplayHauler);
			TestFalse(TEXT("Warehouse closes after its final access session ends"),
				GameplayWarehouse->IsDoorOpenRequested());
		}
	}

	DestroyTestWorld(GameplayWorld);
	return !HasAnyErrors();
}

#endif
