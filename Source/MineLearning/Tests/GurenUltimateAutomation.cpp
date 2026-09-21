#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/CombatDamageSubsystem.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "MineLearning/Manifestation/Guren/GurenQSkillComponent.h"
#include "MineLearning/Manifestation/Guren/GurenUltimateComponent.h"
#include "MineLearning/Interaction/GrabbableComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGurenUltimateLifecycleTest, "MineLearning.GurenUltimate.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGurenUltimateLifecycleTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Initialization);
	if (!TestNotNull(TEXT("World"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APlayerController* Player = World->SpawnActor<APlayerController>();
	Player->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	ACharacter* Character = World->SpawnActor<ACharacter>();
	UHealthComponent* SourceHealth = NewObject<UHealthComponent>(Character);
	SourceHealth->Faction = ECombatFaction::Player;
	SourceHealth->RegisterComponent();
	UCombatComponent* Combat = NewObject<UCombatComponent>(Character);
	UCombatConfig* Config = NewObject<UCombatConfig>(Combat);
	FSkillDamageSpec Spec;
	Spec.SkillId = TEXT("Arrival");
	Spec.BaseDamage = 400.f;
	Config->Skills.Add(Spec);
	Combat->Config = Config;
	Combat->RegisterComponent();
	UGurenQSkillComponent* Q = NewObject<UGurenQSkillComponent>(Character);
	Q->RegisterComponent();
	Player->Possess(Character);
	TestTrue(TEXT("Test pawn is locally controlled"), Character->IsLocallyControlled());
	Character->EnableInput(Player);
	TestTrue(TEXT("Possessed pawn receives input without an extra pushed component"), Character->InputEnabled());
	Character->SetActorLocation(FVector(0, 0, 300));
	UGurenUltimateComponent* Skill = NewObject<UGurenUltimateComponent>(Character);
	Skill->RegisterComponent();
	AActor* Target = World->SpawnActor<AActor>();
	UHealthComponent* TargetHealth = NewObject<UHealthComponent>(Target);
	TargetHealth->RegisterComponent();
	TargetHealth->InitializeHealth(10000.f);
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Target);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Target->SetRootComponent(Mesh);
	Mesh->RegisterComponent();
	Target->SetActorLocation(FVector(600, 0, 300));
	UGrabbableComponent* Grab = NewObject<UGrabbableComponent>(Target);
	Grab->SetupAttachment(Mesh);
	Grab->RegisterComponent();
	Grab->Completion = EGrabCompletion::Restore;
	const FTransform Original = Character->GetActorTransform();
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	TestFalse(TEXT("Self target is rejected without taking control"), Skill->TryStart(Character));
	TestFalse(TEXT("Invalid start does not lock input"), Player->IsMoveInputIgnored());
	for (int32 LastBeat = 1; LastBeat <= 7; ++LastBeat)
	{
		TestTrue(TEXT("Static target without sockets starts"), Skill->TryStart(Target));
		TestTrue(TEXT("Move input held"), Player->IsMoveInputIgnored());
		TestTrue(TEXT("Look input held"), Player->IsLookInputIgnored());
		TestFalse(TEXT("Pawn input is disabled during the cinematic"), Character->InputEnabled());
		TestFalse(TEXT("Reentry refused"), Skill->TryStart(Target));
		TestFalse(TEXT("Protected target cannot take ordinary damage"), Target->CanBeDamaged());
		Skill->HandleBeat(EGurenUltimateStage::Burst);
		TestEqual(TEXT("Out of order result is rejected"), Skill->GetStage(), EGurenUltimateStage::Ready);
		for (int32 Beat = 2; Beat <= LastBeat; ++Beat)
		{
			Skill->HandleBeat(static_cast<EGurenUltimateStage>(Beat));
		}
		Skill->Abort();
		Skill->Abort();
		TestFalse(TEXT("Every abort stage returns idle"), Skill->IsUltimateActive());
		TestFalse(TEXT("Move lock released"), Player->IsMoveInputIgnored());
		TestFalse(TEXT("Look lock released"), Player->IsLookInputIgnored());
		TestTrue(TEXT("Pawn input itself is restored, not just controller locks"), Character->InputEnabled());
		TestTrue(TEXT("Target reservation released"), Grab->CanGrab(Character));
		TestTrue(TEXT("Damage flag restored"), Target->CanBeDamaged());
		TestTrue(TEXT("Original airborne transform restored"), Character->GetActorTransform().Equals(Original));
		TestEqual(TEXT("Original movement mode restored"), Character->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Flying);
	}
	Player->SetIgnoreMoveInput(true);
	TestTrue(TEXT("Start preserves an existing external input lock"), Skill->TryStart(Target));
	Skill->Abort();
	TestTrue(TEXT("External move lock remains after exit"), Player->IsMoveInputIgnored());
	Player->SetIgnoreMoveInput(false);
	TestTrue(TEXT("Target destruction scenario starts"), Skill->TryStart(Target));
	const FVector CachedHit = Skill->GetHitLocation();
	Target->Destroy();
	for (int32 Beat = 2; Beat <= 7; ++Beat)
	{
		Skill->HandleBeat(static_cast<EGurenUltimateStage>(Beat));
	}
	Skill->HandleBeat(EGurenUltimateStage::Idle);
	TestFalse(TEXT("Destroyed target still exits normally"), Skill->IsUltimateActive());
	TestTrue(TEXT("Target destruction preserves burst position"), CachedHit.Equals(Skill->GetHitLocation()));
	TestFalse(TEXT("No lock after target destruction"), Player->IsMoveInputIgnored());
	TestTrue(TEXT("Normal completion restores Pawn input"), Character->InputEnabled());
	const FVector ArcStart(0, 0, 0), ArcEnd(1000, 0, 0);
	TestTrue(TEXT("Transfer starts at the launch point"), UGurenUltimateComponent::EvaluateArc(ArcStart, ArcEnd, FVector::RightVector, 400, 0).Equals(ArcStart));
	TestTrue(TEXT("Transfer passes through its target"), UGurenUltimateComponent::EvaluateArc(ArcStart, ArcEnd, FVector::RightVector, 400, 1).Equals(ArcEnd));
	float PreviousX = -1.f;
	for (int32 Sample = 0; Sample <= 20; ++Sample)
	{
		const FVector Point = UGurenUltimateComponent::EvaluateArc(ArcStart, ArcEnd, FVector::RightVector, 400, Sample / 20.f);
		TestTrue(TEXT("Transfer never reverses, overshoots or changes bow side"), Point.X > PreviousX && Point.X <= ArcEnd.X && Point.Y >= 0.f && Point.Y <= 150.f);
		PreviousX = Point.X;
	}
	TestTrue(TEXT("Initial attack with no bow is direct"), UGurenUltimateComponent::EvaluateArc(ArcStart, ArcEnd, FVector::RightVector, 0, 0.5f).Equals(FVector(500, 0, 0)));
	Skill->FlightSpeedMultiplier = 1.f;
	const float NormalFlightTime = Skill->GetStageDuration(EGurenUltimateStage::Launch);
	const float NormalReturnTime = Skill->GetStageDuration(EGurenUltimateStage::Impact);
	const float PoseTime = Skill->GetStageDuration(EGurenUltimateStage::Arrival);
	Skill->FlightSpeedMultiplier = 1.5f;
	TestTrue(TEXT("1.5x speed shortens outbound and return durations"),
		FMath::IsNearlyEqual(Skill->GetStageDuration(EGurenUltimateStage::Launch) * 1.5f, NormalFlightTime)
		&& FMath::IsNearlyEqual(Skill->GetStageDuration(EGurenUltimateStage::Impact) * 1.5f, NormalReturnTime));
	TestEqual(TEXT("Flight speed does not shorten the hero pose"), Skill->GetStageDuration(EGurenUltimateStage::Arrival), PoseTime);
	const auto MakeMeshActor = [World](FVector Location, bool bTarget)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UStaticMeshComponent* Root = NewObject<UStaticMeshComponent>(Actor);
		Root->SetMobility(EComponentMobility::Movable);
		Root->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Root->SetCollisionProfileName(TEXT("BlockAll"));
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		Actor->SetActorLocation(Location);
		if (bTarget)
		{
			UGrabbableComponent* Grabbable = NewObject<UGrabbableComponent>(Actor);
			Grabbable->SetupAttachment(Root);
			Grabbable->RegisterComponent();
			UHealthComponent* Health = NewObject<UHealthComponent>(Actor);
			Health->RegisterComponent();
			Health->InitializeHealth(5000.f);
		}
		return Actor;
	};
	AActor* Floor = MakeMeshActor(FVector(0, 0, -50), false);
	Floor->SetActorScale3D(FVector(100, 100, 1));
	AActor* First = MakeMeshActor(FVector(600, 0, 300), true);
	AActor* Second = MakeMeshActor(FVector(900, 500, 300), true);
	AActor* Third = MakeMeshActor(FVector(1000, -600, 300), true);
	UCombatDamageSubsystem* Damage = World->GetSubsystem<UCombatDamageSubsystem>();
	Damage->ApplyDebugDamage(First, 2500.f);
	Damage->ApplyDebugDamage(Second, 2499.f);
	Third->FindComponentByClass<UHealthComponent>()->InitializeHealth(1800.f);
	Damage->ApplyDebugDamage(Third, 800.f);
	Character->SetActorLocation(FVector(0, 0, 300));
	Skill->MaxTargets = 3;
	Skill->FinalHoverHeight = 1650.f;
	TestTrue(TEXT("Multi-target cast starts"), Skill->TryStart(First));
	TestEqual(TEXT("Three distinct targets are reserved"), Skill->GetTargetCount(), 3);
	Skill->TickComponent(Skill->GetStageDuration(EGurenUltimateStage::Ready), LEVELTICK_All, nullptr);
	TestTrue(TEXT("Ground and airborne starts share fixed ground clearance"), FMath::IsNearlyEqual(Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Skill->LaunchHeight, 1.f));
	Skill->TickComponent(Skill->GetStageDuration(EGurenUltimateStage::Launch), LEVELTICK_All, nullptr);
	TestTrue(TEXT("Every waypoint is pierced before return"), !Skill->GetTargets().ContainsByPredicate([](const FArrivalTarget& Entry) { return !Entry.bPierced; }));
	TestFalse(TEXT("No damage settlement before pose"), First->IsActorBeingDestroyed() || Second->IsActorBeingDestroyed() || Third->IsActorBeingDestroyed());
	for (int32 Beat = 4; Beat <= 5; ++Beat)
	{
		Skill->HandleBeat(static_cast<EGurenUltimateStage>(Beat));
	}
	TestFalse(TEXT("Targets remain present in the pose close-up"), First->IsActorBeingDestroyed());
	TestTrue(TEXT("Final pose obeys its separately configured ground clearance"), FMath::IsNearlyEqual(Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Skill->FinalHoverHeight, 1.f));
	Skill->HandleBeat(EGurenUltimateStage::Burst);
	TestTrue(TEXT("Percent boundary and fixed boundary both execute"), First->IsActorBeingDestroyed() && Third->IsActorBeingDestroyed());
	TestFalse(TEXT("Above-threshold target survives"), Second->IsActorBeingDestroyed());
	TestEqual(TEXT("Arrival uses pre-hit HP, even if its damage crosses the threshold"), Second->FindComponentByClass<UHealthComponent>()->GetHealth(), 2101.f);
	TestEqual(TEXT("Survivor does not receive dissolve authorization"), Skill->GetTargets().FilterByPredicate([](const FArrivalTarget& Entry) { return Entry.bExecuteEligible; }).Num(), 2);
	Second->Destroy();
	Skill->HandleBeat(EGurenUltimateStage::Recover);
	Skill->HandleBeat(EGurenUltimateStage::Idle);
	TestTrue(TEXT("Multi-target completion restores Pawn input"), Character->InputEnabled());
	AActor* Preserved = MakeMeshActor(FVector(600, 0, 300), true);
	Character->DisableInput(Player);
	TestTrue(TEXT("Externally disabled Pawn can be invoked by gameplay"), Skill->TryStart(Preserved));
	Skill->Abort();
	TestFalse(TEXT("Original disabled Pawn input is preserved"), Character->InputEnabled());
	Preserved->Destroy();
	Floor->Destroy();
	Character->Destroy();
	Player->Destroy();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
