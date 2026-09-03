#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "MineLearning/AI/HaulerCharacter.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/Mining/OreProcessorMachine.h"
#include "MineLearning/Mining/SellStation.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/Navigation/NavigationStandards.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "PhysicsEngine/BodySetup.h"

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
		bool bRequireEndpointClearance = false;
	};

	bool IsRuntimeDoorComponent(const UPrimitiveComponent* Component)
	{
		if (!Component)
		{
			return false;
		}

		const FName Name = Component->GetFName();
		return Name == TEXT("Door")
			|| Name == TEXT("BarrierVisual")
			|| Name == TEXT("BC_Warehouse_DoorSafetyBlocker");
	}

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

	void CheckNavigationCollisionParity(FAutomationTestBase& Test, AActor* Actor)
	{
		if (!Actor)
		{
			return;
		}

		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Actor);
		for (UPrimitiveComponent* Component : PrimitiveComponents)
		{
			if (!Component)
			{
				continue;
			}

			const FString Context = FString::Printf(
				TEXT("%s.%s"),
				*Actor->GetActorLabel(),
				*Component->GetName());
			const ECollisionEnabled::Type CollisionEnabled = Component->GetCollisionEnabled();
			const bool bQueriesCollision = CollisionEnabled == ECollisionEnabled::QueryOnly
				|| CollisionEnabled == ECollisionEnabled::QueryAndPhysics
				|| CollisionEnabled == ECollisionEnabled::QueryAndProbe;
			const bool bBlocksPawn = bQueriesCollision
				&& Component->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;

			if (IsRuntimeDoorComponent(Component))
			{
				Test.TestFalse(
					FString::Printf(TEXT("%s runtime door is excluded from static NavMesh"), *Context),
					Component->CanEverAffectNavigation());
			}
			else if (bBlocksPawn)
			{
				Test.TestTrue(
					FString::Printf(TEXT("%s Pawn blocker affects navigation"), *Context),
					Component->CanEverAffectNavigation());
			}
			else
			{
				Test.TestFalse(
					FString::Printf(TEXT("%s non-blocker is excluded from navigation"), *Context),
					Component->CanEverAffectNavigation());
			}
		}
	}

	void CheckExactStaticCollision(FAutomationTestBase& Test, const TCHAR* AssetPath)
	{
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, AssetPath);
		if (!Test.TestNotNull(
			FString::Printf(TEXT("%s static mesh"), AssetPath),
			Mesh))
		{
			return;
		}

		const UBodySetup* BodySetup = Mesh->GetBodySetup();
		if (!Test.TestNotNull(
			FString::Printf(TEXT("%s body setup"), AssetPath),
			BodySetup))
		{
			return;
		}

		Test.TestEqual(
			FString::Printf(TEXT("%s does not use a protruding convex hull"), AssetPath),
			BodySetup->CollisionTraceFlag,
			CTF_UseComplexAsSimple);
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
	AOreProcessorMachine* Processor = FindFirstActor<AOreProcessorMachine>(World);
	ASellStation* SellStation = FindFirstActor<ASellStation>(World);
	AWarehouseDepot* Warehouse = FindFirstActor<AWarehouseDepot>(World);
	ARecastNavMesh* RecastNavMesh = FindFirstActor<ARecastNavMesh>(World);
	USceneComponent* AccessRamp = FindSceneComponent(World, TEXT("AccessRamp"));
	USceneComponent* FormPad = FindSceneComponent(World, TEXT("FormPad"));
	USceneComponent* ProcessorInputPoint = FindSceneComponent(World, TEXT("InputPoint"));
	USceneComponent* ProcessorOutput = FindSceneComponent(World, TEXT("OutputPoint"));
	USceneComponent* SellApproach = FindSceneComponent(World, TEXT("RobotApproachPoint"));
	USceneComponent* PaymentDropPoint = FindSceneComponent(World, TEXT("PaymentDropPoint"));
	USceneComponent* CarrierWaitPoint = FindSceneComponent(World, TEXT("CarrierWaitPoint"));
	if (!TestNotNull(TEXT("Hauler"), Hauler)
		|| !TestNotNull(TEXT("OreBuddy"), OreBuddy)
		|| !TestNotNull(TEXT("Processor"), Processor)
		|| !TestNotNull(TEXT("Sell Station"), SellStation)
		|| !TestNotNull(TEXT("Warehouse"), Warehouse)
		|| !TestNotNull(TEXT("Recast NavMesh"), RecastNavMesh)
		|| !TestNotNull(TEXT("Robot Center AccessRamp"), AccessRamp)
		|| !TestNotNull(TEXT("Robot Center FormPad"), FormPad)
		|| !TestNotNull(TEXT("Processor InputPoint"), ProcessorInputPoint)
		|| !TestNotNull(TEXT("Processor OutputPoint"), ProcessorOutput)
		|| !TestNotNull(TEXT("Sell Station RobotApproachPoint"), SellApproach)
		|| !TestNotNull(TEXT("Robot Center PaymentDropPoint"), PaymentDropPoint)
		|| !TestNotNull(TEXT("Robot Center CarrierWaitPoint"), CarrierWaitPoint))
	{
		return false;
	}

	const FNavDataConfig& NavConfig = RecastNavMesh->GetConfig();
	TestTrue(TEXT("Shared NavMesh radius covers the largest ground pawn"),
		NavConfig.AgentRadius >= MineLearningNavigation::SharedAgentRadius);
	TestTrue(TEXT("Shared NavMesh height covers the tallest ground pawn"),
		NavConfig.AgentHeight >= MineLearningNavigation::SharedAgentHeight);
	TestTrue(TEXT("Shared NavMesh only connects authored ground steps"),
		FMath::IsNearlyEqual(
			RecastNavMesh->GetAgentMaxStepHeight(ENavigationDataResolution::Default),
			MineLearningNavigation::MaxGroundStepHeight));

	const TArray<UClass*> GroundPawnClasses = {
		StaticLoadClass(ACharacter::StaticClass(), nullptr,
			TEXT("/Game/MineLearning/Player/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C")),
		StaticLoadClass(ACharacter::StaticClass(), nullptr,
			TEXT("/Game/MineLearning/Characters/OreBuddy/Blueprints/BP_OreBuddy07.BP_OreBuddy07_C")),
		StaticLoadClass(ACharacter::StaticClass(), nullptr,
			TEXT("/Game/MineLearning/Characters/Gunner/Blueprints/BP_Gunner.BP_Gunner_C")),
		StaticLoadClass(ACharacter::StaticClass(), nullptr,
			TEXT("/Game/MineLearning/Mining/Logistics/Blueprints/BP_Hauler.BP_Hauler_C")),
	};
	for (const UClass* GroundPawnClass : GroundPawnClasses)
	{
		const ACharacter* CharacterCDO = GroundPawnClass
			? Cast<ACharacter>(GroundPawnClass->GetDefaultObject())
			: nullptr;
		const UCapsuleComponent* Capsule = CharacterCDO ? CharacterCDO->GetCapsuleComponent() : nullptr;
		const UCharacterMovementComponent* Movement = CharacterCDO
			? CharacterCDO->GetCharacterMovement()
			: nullptr;
		if (!TestNotNull(
			FString::Printf(TEXT("%s capsule"), *GetNameSafe(GroundPawnClass)),
			Capsule)
			|| !TestNotNull(
				FString::Printf(TEXT("%s movement"), *GetNameSafe(GroundPawnClass)),
				Movement))
		{
			continue;
		}

		TestTrue(
			FString::Printf(TEXT("%s fits shared NavMesh radius"), *GetNameSafe(GroundPawnClass)),
			Capsule->GetScaledCapsuleRadius() <= MineLearningNavigation::SharedAgentRadius);
		TestTrue(
			FString::Printf(TEXT("%s fits shared NavMesh height"), *GetNameSafe(GroundPawnClass)),
			Capsule->GetScaledCapsuleHalfHeight() * 2.0f <= MineLearningNavigation::SharedAgentHeight);
		TestTrue(
			FString::Printf(TEXT("%s can traverse authored steps"), *GetNameSafe(GroundPawnClass)),
			Movement->MaxStepHeight >= MineLearningNavigation::CharacterStepHeight);
	}

	CheckNavigationCollisionParity(*this, Processor);
	CheckNavigationCollisionParity(*this, SellStation);
	CheckNavigationCollisionParity(*this, Warehouse);
	CheckNavigationCollisionParity(*this, PaymentDropPoint->GetOwner());

	const TArray<const TCHAR*> ExactStaticCollisionAssets = {
		TEXT("/Game/MineLearning/Mining/Processing/Resource/SM_OreProcessor_Body"),
		TEXT("/Game/MineLearning/Mining/Storage/Resource/SM_Warehouse_Body"),
		TEXT("/Game/MineLearning/Mining/SellStation/SM_SellStation_Body"),
		TEXT("/Game/MineLearning/Environment/RobotCenter/Meshes/SM_RobotCenter_Base"),
		TEXT("/Game/MineLearning/Environment/RobotCenter/Meshes/SM_RobotCenter_AccessRamp"),
		TEXT("/Game/MineLearning/Environment/RobotCenter/Meshes/SM_RobotCenter_RobotExitRamp"),
		TEXT("/Game/MineLearning/Environment/RobotCenter/Meshes/SM_RobotCenter_MainFrame"),
		TEXT("/Game/MineLearning/Environment/RobotCenter/Meshes/SM_RobotCenter_MainCore"),
		TEXT("/Game/MineLearning/Environment/RobotCenter/Meshes/SM_RobotCenter_ShopTerminal"),
		TEXT("/Game/MineLearning/Environment/RobotCenter/Meshes/SM_RobotCenter_ShopDisplayPad"),
		TEXT("/Game/MineLearning/Environment/RobotCenter/Meshes/SM_RobotCenter_FormPad"),
		TEXT("/Game/MineLearning/Environment/RobotCenter/Meshes/SM_RobotCenter_FormTerminal"),
		TEXT("/Game/MineLearning/Environment/MiningArea/Meshes/SM_MineCliff_A"),
		TEXT("/Game/MineLearning/Environment/MiningArea/Meshes/SM_MineCliff_B"),
		TEXT("/Game/MineLearning/Environment/MiningArea/Meshes/SM_MineCliff_End_A"),
		TEXT("/Game/MineLearning/Environment/MiningArea/Meshes/SM_MineCliff_Corner_90"),
		TEXT("/Game/MineLearning/Environment/MiningArea/Meshes/SM_MiningSlot_Base_A"),
	};
	for (const TCHAR* AssetPath : ExactStaticCollisionAssets)
	{
		CheckExactStaticCollision(*this, AssetPath);
	}

	const FVector HaulerStart = Hauler->GetActorLocation();
	const FVector WarehouseDock = Warehouse->GetDeliveryPointWorldTransform().GetLocation();
	const FVector ProcessorInput = ProcessorInputPoint->GetComponentLocation();
	const FVector RobotCenterPaymentDrop = PaymentDropPoint->GetComponentLocation();
	const FBox FormPadBounds = CastChecked<UPrimitiveComponent>(FormPad)->Bounds.GetBox();
	const FVector FormPadSurface(
		FormPadBounds.GetCenter().X,
		FormPadBounds.GetCenter().Y,
		FormPadBounds.Max.Z);

	const TArray<FRouteCheck> RouteChecks = {
		{TEXT("Industrial to Mining Pit"), FVector(0.0f, 600.0f, 250.0f), FVector(0.0f, -1250.0f, 0.0f), Hauler},
		{TEXT("Hauler Main Ramp"), HaulerStart, FVector(0.0f, -1250.0f, 0.0f), Hauler},
		{TEXT("Hauler to Warehouse Dock"), HaulerStart, WarehouseDock, Hauler, true},
		{TEXT("Hauler to Processor Input"), HaulerStart, ProcessorInput, Hauler, true},
		{TEXT("Hauler to Processor Output"), HaulerStart, ProcessorOutput->GetComponentLocation(), Hauler, true},
		{TEXT("Hauler to Sell Station"), HaulerStart, SellApproach->GetComponentLocation(), Hauler, true},
		{TEXT("Warehouse Dock to PaymentDropPoint"), WarehouseDock, RobotCenterPaymentDrop, Hauler, true},
		{TEXT("PaymentDropPoint to CarrierWaitPoint"), RobotCenterPaymentDrop, CarrierWaitPoint->GetComponentLocation(), Hauler, true},
		{TEXT("Robot Center Access Ramp"), FVector(0.0f, 1250.0f, 250.0f), FVector(0.0f, 1775.0f, 328.0f), OreBuddy},
		{TEXT("Robot Center Transform Pad"), FVector(0.0f, 1850.0f, 328.0f), FormPadSurface, OreBuddy, true},
		{TEXT("Ore A Approach"), FVector(0.0f, -1250.0f, 0.0f), FVector(-800.0f, -650.0f, 28.0f), OreBuddy},
		{TEXT("Ore B Approach"), FVector(0.0f, -1250.0f, 0.0f), FVector(800.0f, -650.0f, 28.0f), OreBuddy},
		{TEXT("Ore C Approach"), FVector(0.0f, -1250.0f, 0.0f), FVector(0.0f, -1400.0f, 28.0f), OreBuddy},
		{TEXT("Ore A Ring"), FVector(-1050.0f, -850.0f, 28.0f), FVector(-550.0f, -850.0f, 28.0f), OreBuddy},
		{TEXT("Ore B Ring"), FVector(550.0f, -850.0f, 28.0f), FVector(1050.0f, -850.0f, 28.0f), OreBuddy},
		{TEXT("Ore C Ring"), FVector(-250.0f, -1600.0f, 28.0f), FVector(250.0f, -1600.0f, 28.0f), OreBuddy},
	};

	const FVector ProjectionExtent(130.0f, 130.0f, 250.0f);
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

		if (!Route.bRequireEndpointClearance)
		{
			continue;
		}

		const FVector CapsuleCenter = ProjectedEnd.Location
			+ FVector(0.0f, 0.0f, MineLearningNavigation::SharedAgentHeight * 0.5f + 2.0f);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WorldLayoutNavigationEndpoint), false);
		if (Route.PathfindingContext)
		{
			QueryParams.AddIgnoredActor(Route.PathfindingContext);
		}
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByChannel(
			Overlaps,
			CapsuleCenter,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeCapsule(
				MineLearningNavigation::SharedAgentRadius,
				MineLearningNavigation::SharedAgentHeight * 0.5f),
			QueryParams);
		bool bEndpointBlocked = false;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			const UPrimitiveComponent* Component = Overlap.Component.Get();
			if (!Component
				|| IsRuntimeDoorComponent(Component)
				|| Component->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block)
			{
				continue;
			}

			bEndpointBlocked = true;
			AddInfo(FString::Printf(
				TEXT("%s clearance blocked by %s.%s"),
				Route.Name,
				*GetNameSafe(Component->GetOwner()),
				*Component->GetName()));
		}
		TestFalse(FString::Printf(TEXT("%s end has full capsule clearance"), Route.Name), bEndpointBlocked);
	}

	return !HasAnyErrors();
}

#endif
