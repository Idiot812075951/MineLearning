#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "MineLearning/Mining/ItemLogisticsLibrary.h"
#include "MineLearning/Mining/ItemTypes.h"
#include "MineLearning/Mining/OreProcessorMachine.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/Mining/SellStation.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/UI/WarehouseScreenWidgetBase.h"
#include "Blueprint/UserWidget.h"
#include "Engine/StaticMesh.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWarehouseReservationTest,
	"MineLearning.Warehouse.ReservationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWarehouseReservationTest::RunTest(const FString& Parameters)
{
	UResourceStorageComponent* Storage = NewObject<UResourceStorageComponent>();
	if (!TestNotNull(TEXT("Transient warehouse storage"), Storage))
	{
		return false;
	}

	const FItemStack TenOre{EItemType::IronOre, 10};
	const FItemStack FiveOre{EItemType::IronOre, 5};
	const FItemStack ThreeOre{EItemType::IronOre, 3};
	const FItemStack TwoOre{EItemType::IronOre, 2};
	const FItemStack SixOre{EItemType::IronOre, 6};

	TestTrue(TEXT("Warehouse accepts ten ore"), Storage->AddItem(TenOre));
	TestTrue(TEXT("Player can freeze five available ore"), Storage->TryReserveItem(FiveOre));
	TestEqual(TEXT("Freezing keeps physical total"), Storage->GetStoredItemAmount(EItemType::IronOre), 10);
	TestEqual(TEXT("Five ore are frozen"), Storage->GetReservedItemAmount(EItemType::IronOre), 5);
	TestEqual(TEXT("Only five ore remain available"), Storage->GetAvailableItemAmount(EItemType::IronOre), 5);
	TestFalse(TEXT("Player cannot sell six more ore"), Storage->TryReserveItem(SixOre));
	TestFalse(TEXT("Unreserved removal cannot consume frozen ore"), Storage->RemoveItem(SixOre));

	TestTrue(TEXT("Carrier pickup commits three frozen ore"), Storage->CommitReservedItem(ThreeOre));
	TestEqual(TEXT("Carrier pickup removes physical stock"), Storage->GetStoredItemAmount(EItemType::IronOre), 7);
	TestEqual(TEXT("Two ore remain frozen"), Storage->GetReservedItemAmount(EItemType::IronOre), 2);
	TestEqual(TEXT("Five ore remain available after pickup"), Storage->GetAvailableItemAmount(EItemType::IronOre), 5);
	TestTrue(TEXT("Player can cancel the remaining order"), Storage->ReleaseReservedItem(TwoOre));
	TestEqual(TEXT("Cancellation restores all remaining stock"), Storage->GetAvailableItemAmount(EItemType::IronOre), 7);
	TestEqual(TEXT("Iron ore has a configured sale value"), UItemLogisticsLibrary::GetUnitSellPrice(EItemType::IronOre), 1);

	const FItemStack ThreeIngots{EItemType::IronIngot, 3};
	TestTrue(TEXT("Warehouse accepts processed iron ingots"), Storage->AddItem(ThreeIngots));
	TestEqual(TEXT("Processed ingots remain available for hauling"), Storage->GetAvailableItemAmount(EItemType::IronIngot), 3);
	TestEqual(
		TEXT("Iron ingot uses the processed-material routing category"),
		UItemLogisticsLibrary::GetItemCategory(EItemType::IronIngot),
		EItemCategory::ProcessedMaterial);

	const TArray<TPair<FString, FString>> StandardizedMeshes = {
		{TEXT("Iron ore"), TEXT("/Game/MineLearning/Mining/Ores/Iron/Meshes/SM_Ore_Iron_Drop_01.SM_Ore_Iron_Drop_01")},
		{TEXT("Coin"), TEXT("/Game/MineLearning/Mining/Resources/Coin/SM_GoldCoin.SM_GoldCoin")},
		{TEXT("Iron ingot"), TEXT("/Game/MineLearning/Mining/Resources/IronIngot/SM_IronIngot.SM_IronIngot")}};

	for (const TPair<FString, FString>& Entry : StandardizedMeshes)
	{
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Entry.Value);
		if (!TestNotNull(*FString::Printf(TEXT("%s mesh is loadable"), *Entry.Key), Mesh))
		{
			continue;
		}

		const FVector WorldSize = MineLearningItemVisual::GetWorldSize(Mesh);
		const float MaxDimension = FMath::Max3(WorldSize.X, WorldSize.Y, WorldSize.Z);
		TestTrue(
			*FString::Printf(TEXT("%s is normalized to the shared 30 cm visual envelope"), *Entry.Key),
			FMath::IsNearlyEqual(MaxDimension, MineLearningItemVisual::StandardMaxDimensionCm, 0.1f));
	}

	TestNotNull(
		TEXT("Sell station Blueprint is loadable"),
		LoadClass<ASellStation>(
			nullptr,
			TEXT("/Game/MineLearning/Mining/SellStation/BP_SellStation.BP_SellStation_C")));
	UClass* WarehouseWidgetClass = LoadClass<UWarehouseScreenWidgetBase>(
		nullptr,
		TEXT("/Game/MineLearning/Mining/UI/WBP_Warehouse.WBP_Warehouse_C"));
	TestNotNull(TEXT("Warehouse UMG Blueprint is loadable"), WarehouseWidgetClass);
	if (WarehouseWidgetClass)
	{
		UWarehouseScreenWidgetBase* WarehouseWidget =
			NewObject<UWarehouseScreenWidgetBase>(
				GetTransientPackage(), WarehouseWidgetClass);
		TestNotNull(TEXT("Warehouse UI selection state can be instantiated"), WarehouseWidget);
		if (WarehouseWidget)
		{
			TestEqual(
				TEXT("A new warehouse selection begins at one"),
				WarehouseWidget->UpdateSelectionState(EItemType::IronOre, 5, 0),
				1);
			TestEqual(
				TEXT("One plus click selects exactly two"),
				WarehouseWidget->StepSelectionAmount(1),
				2);
			TestEqual(
				TEXT("Inventory refresh preserves the selected amount"),
				WarehouseWidget->UpdateSelectionState(EItemType::IronOre, 5, 0),
				2);
			TestEqual(
				TEXT("Maximum selection uses the available amount"),
				WarehouseWidget->MaximizeSelectionAmount(),
				5);
			TestEqual(
				TEXT("A different item resets selection to one"),
				WarehouseWidget->UpdateSelectionState(EItemType::Coin, 8, 0),
				1);
		}
	}

	UClass* WarehouseClass = LoadClass<AWarehouseDepot>(
		nullptr,
		TEXT("/Game/MineLearning/Mining/Storage/Blueprints/BP_Warehouse.BP_Warehouse_C"));
	TestNotNull(TEXT("Warehouse Blueprint is loadable"), WarehouseClass);
	if (WarehouseClass)
	{
		const FFloatProperty* DoorOpenRollProperty = FindFProperty<FFloatProperty>(
			WarehouseClass, TEXT("DoorOpenRoll"));
		TestNotNull(TEXT("Warehouse exposes its authored door angle"), DoorOpenRollProperty);
		if (DoorOpenRollProperty)
		{
			const float DoorOpenRoll = DoorOpenRollProperty->GetPropertyValue_InContainer(
				WarehouseClass->GetDefaultObject());
			TestEqual(
				TEXT("Warehouse door uses the authored positive roll"),
				DoorOpenRoll,
				90.0f);
		}
	}
	TestNotNull(
		TEXT("Ore processor Blueprint is loadable"),
		LoadClass<AOreProcessorMachine>(
			nullptr,
			TEXT("/Game/MineLearning/Mining/Processing/Blueprints/BP_ProcesserMachine.BP_ProcesserMachine_C")));

	return !HasAnyErrors();
}

#endif
