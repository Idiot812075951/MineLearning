#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "MineLearning/AI/HaulerCharacter.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

namespace WorldLayoutP021Navigation
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

	struct FRouteCheck
	{
		const TCHAR* Name;
		FVector Start;
		FVector End;
		AActor* PathfindingContext = nullptr;
	};

	template<typename TActorType>
	TActorType* FindFirstActor(UWorld* World)
	{
		for (TActorIterator<TActorType> ActorIterator(World); ActorIterator; ++ActorIterator)
		{
			return *ActorIterator;
		}

		return nullptr;
	}

	USceneComponent* FindSceneComponent(UWorld* World, FName ComponentName)
	{
		for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
		{
			TInlineComponentArray<USceneComponent*> SceneComponents(*ActorIterator);
			for (USceneComponent* SceneComponent : SceneComponents)
			{
				if (SceneComponent && SceneComponent->GetFName() == ComponentName)
				{
					return SceneComponent;
				}
			}
		}

		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldLayoutP021NavigationTest,
	"MineLearning.WorldLayout.P02_1.NavigationRegression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldLayoutP021NavigationTest::RunTest(const FString& Parameters)
{
	using namespace WorldLayoutP021Navigation;

	UWorld* World = FindCurrentEditorWorld();
	if (!TestNotNull(TEXT("Current editor world"), World))
	{
		return false;
	}

	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(World);
	if (!TestNotNull(TEXT("Navigation system"), NavigationSystem))
	{
		return false;
	}

	// The P02.1 cleanup changes collision-bearing terrain shoulders, so force a
	// deterministic refresh before checking connectivity. This is an editor-only
	// regression helper and does not change runtime navigation policy.
	NavigationSystem->Build();

	AHaulerCharacter* Hauler = FindFirstActor<AHaulerCharacter>(World);
	AMiningCompanionCharacter* OreBuddy = FindFirstActor<AMiningCompanionCharacter>(World);
	AWarehouseDepot* Warehouse = FindFirstActor<AWarehouseDepot>(World);
	USceneComponent* PaymentDropPoint = FindSceneComponent(World, TEXT("PaymentDropPoint"));
	if (!TestNotNull(TEXT("Hauler"), Hauler)
		|| !TestNotNull(TEXT("OreBuddy"), OreBuddy)
		|| !TestNotNull(TEXT("Warehouse"), Warehouse)
		|| !TestNotNull(TEXT("Robot Center PaymentDropPoint"), PaymentDropPoint))
	{
		return false;
	}

	const FVector HaulerStart = Hauler->GetActorLocation();
	const FVector WarehouseDock = Warehouse->GetDeliveryPointWorldTransform().GetLocation();
	const FVector RobotCenterPaymentDrop = PaymentDropPoint->GetComponentLocation();

	const TArray<FRouteCheck> RouteChecks = {
		{TEXT("Industrial to Mining Pit"), FVector(0.0f, 600.0f, 250.0f), FVector(0.0f, -1250.0f, 0.0f), Hauler},
		{TEXT("Hauler Main Ramp"), HaulerStart, FVector(0.0f, -1250.0f, 0.0f), Hauler},
		{TEXT("Hauler to Warehouse Dock"), HaulerStart, WarehouseDock, Hauler},
		{TEXT("Warehouse Dock to PaymentDropPoint"), WarehouseDock, RobotCenterPaymentDrop, Hauler},
		{TEXT("Ore A Approach"), FVector(0.0f, -1250.0f, 0.0f), FVector(-800.0f, -650.0f, 28.0f), OreBuddy},
		{TEXT("Ore B Approach"), FVector(0.0f, -1250.0f, 0.0f), FVector(800.0f, -650.0f, 28.0f), OreBuddy},
		{TEXT("Ore C Approach"), FVector(0.0f, -1250.0f, 0.0f), FVector(0.0f, -1400.0f, 28.0f), OreBuddy},
		{TEXT("Ore A Ring"), FVector(-1050.0f, -850.0f, 28.0f), FVector(-550.0f, -850.0f, 28.0f), OreBuddy},
		{TEXT("Ore B Ring"), FVector(550.0f, -850.0f, 28.0f), FVector(1050.0f, -850.0f, 28.0f), OreBuddy},
		{TEXT("Ore C Ring"), FVector(-250.0f, -1600.0f, 28.0f), FVector(250.0f, -1600.0f, 28.0f), OreBuddy},
	};

	const FVector ProjectionExtent(180.0f, 180.0f, 300.0f);
	for (const FRouteCheck& Route : RouteChecks)
	{
		FNavLocation ProjectedStart;
		FNavLocation ProjectedEnd;
		const bool bStartOnNavigation = NavigationSystem->ProjectPointToNavigation(
			Route.Start, ProjectedStart, ProjectionExtent);
		const bool bEndOnNavigation = NavigationSystem->ProjectPointToNavigation(
			Route.End, ProjectedEnd, ProjectionExtent);

		TestTrue(FString::Printf(TEXT("%s start is on NavMesh"), Route.Name), bStartOnNavigation);
		TestTrue(FString::Printf(TEXT("%s end is on NavMesh"), Route.Name), bEndOnNavigation);
		if (!bStartOnNavigation || !bEndOnNavigation)
		{
			continue;
		}

		UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
			World,
			ProjectedStart.Location,
			ProjectedEnd.Location,
			Route.PathfindingContext);
		const bool bHasCompletePath = Path && Path->IsValid() && !Path->IsPartial();
		TestTrue(FString::Printf(TEXT("%s has a complete path"), Route.Name), bHasCompletePath);
	}

	return !HasAnyErrors();
}

#endif
