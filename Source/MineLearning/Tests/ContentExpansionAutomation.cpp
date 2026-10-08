#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/ChargedMiningEffect.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/MiningToolComponent.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MineLearning/AI/SharedCarryTask.h"
#include "MineLearning/AI/CooperativeHaulingComponent.h"
#include "MineLearning/AI/HaulerCharacter.h"
#include "MineLearning/AI/HaulerAIController.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCarryContributionsTest, "MineLearning.Expansion.CapacityAndTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarryContributionsTest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	AActor* First = World->SpawnActor<AActor>();
	AActor* Second = World->SpawnActor<AActor>();
	UResourceCarryComponent* Source = NewObject<UResourceCarryComponent>(First);
	First->AddInstanceComponent(Source); Source->RegisterComponent();
	UResourceCarryComponent* Target = NewObject<UResourceCarryComponent>(Second);
	Second->AddInstanceComponent(Target); Target->RegisterComponent();
	Source->ConfigureAcceptance(20, true, {});
	Target->ConfigureAcceptance(4, true, {});
	Source->AddItem({EItemType::IronOre, 20});
	Target->SetCapacityBonus(TEXT("Team"), 0.25f);
	TestEqual(TEXT("Real capacity 4 -> 5"), Target->GetCapacity(), 5);
	Target->SetCapacityBonus(TEXT("Focus"), 0.75f);
	TestEqual(TEXT("Focus is +100 total, not +125"), Target->GetCapacity(), 8);
	Target->SetCapacityBonus(TEXT("Cargo"), 1.f);
	TestEqual(TEXT("Capacity shares additive percentage layer"), Target->GetCapacity(), 12);
	TestEqual(TEXT("Transfer commits only receiver capacity"), Source->TransferTo(Target), 12);
	TestEqual(TEXT("Source retains remainder"), Source->GetCurrentItemCount(), 8);
	Target->RemoveCapacityBonus(TEXT("Cargo"));
	Target->RemoveCapacityBonus(TEXT("Focus"));
	Target->RemoveCapacityBonus(TEXT("Team"));
	TestEqual(TEXT("Losing capacity never deletes existing cargo"), Target->GetCurrentItemCount(), 12);
	TestTrue(TEXT("Overloaded receiver rejects additional cargo"), Target->IsFull());
	TestEqual(TEXT("Rejected transfer leaves source unchanged"), Source->TransferTo(Target), 0);
	TestEqual(TEXT("Return transfer preserves all 20 items"), Target->TransferTo(Source), 12);
	TestEqual(TEXT("No duplication across repeated transfer"), Source->GetCurrentItemCount(), 20);
	TestEqual(TEXT("Cannot transfer to self"), Source->TransferTo(Source), 0);
	TestFalse(TEXT("Invalid modifier rejected"), Target->SetCapacityBonus(NAME_None, 1.f));
	First->Destroy(); Second->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChargedMiningCycleTest, "MineLearning.Expansion.ChargedWholeCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChargedMiningCycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	AActor* Unit = World->SpawnActor<AActor>();
	UCombatComponent* Combat = NewObject<UCombatComponent>(Unit);
	Unit->AddInstanceComponent(Combat); Combat->RegisterComponent();
	UMiningToolComponent* Mining = NewObject<UMiningToolComponent>(Unit);
	Unit->AddInstanceComponent(Mining); Mining->RegisterComponent();
	UUnitEffectComponent* Effects = NewObject<UUnitEffectComponent>(Unit);
	Unit->AddInstanceComponent(Effects); Effects->RegisterComponent();
	Unit->DispatchBeginPlay();
	UChargedMiningEffectDefinition* Definition = NewObject<UChargedMiningEffectDefinition>();
	Definition->Rule.Id = TEXT("ChargedTest");
	Definition->ChargeSeconds = 0.1f;
	TestTrue(TEXT("Attach charge to mining capability"), Effects->GrantDefinition(TEXT("Charge"), Definition));
	for (int32 Frame = 0; Frame < 8; ++Frame) { ++GFrameCounter; World->GetTimerManager().Tick(0.025f); }
	Mining->OnCycleStarted.Broadcast();
	// A rejected hit has prepare but no commit: charge must survive the empty cycle.
	FMiningHitContext Empty; Mining->OnPrepareHit.Broadcast(Empty);
	Mining->OnCycleFinished.Broadcast();
	Mining->OnCycleStarted.Broadcast();
	for (int32 Segment = 0; Segment < 5; ++Segment)
	{
		FMiningHitContext Hit; Mining->OnPrepareHit.Broadcast(Hit);
		TestEqual(TEXT("Every segment receives double damage"), Hit.DamageMultiplier, 2.f);
		TestEqual(TEXT("Every segment receives empowered impact"), Hit.ImpactScale, 1.5f);
		Mining->OnHitCommitted.Broadcast();
		TestTrue(TEXT("Drill glow stays until cycle ends"), Effects->GetActiveEffectViews()[0].bToolGlow);
		TestEqual(TEXT("Charge never adds a whole-body overlay"), Effects->GetActiveEffectViews()[0].AuraIntensity, 0.f);
	}
	Mining->OnCycleFinished.Broadcast();
	TestFalse(TEXT("Drill glow removed at cycle boundary"), Effects->GetActiveEffectViews()[0].bToolGlow);
	Mining->OnCycleStarted.Broadcast();
	FMiningHitContext Normal; Mining->OnPrepareHit.Broadcast(Normal);
	TestEqual(TEXT("Next cycle starts uncharged"), Normal.DamageMultiplier, 1.f);
	Mining->OnHitCommitted.Broadcast(); // This fills the short test charge during the cycle.
	FMiningHitContext MidCycle; Mining->OnPrepareHit.Broadcast(MidCycle);
	TestEqual(TEXT("Charge completed mid-cycle waits for next whole attack"), MidCycle.DamageMultiplier, 1.f);
	Mining->OnCycleFinished.Broadcast();
	Mining->OnCycleStarted.Broadcast();
	FMiningHitContext Next; Mining->OnPrepareHit.Broadcast(Next);
	TestEqual(TEXT("Ready charge applies to next complete attack"), Next.DamageMultiplier, 2.f);
	Effects->RevokeDefinition(TEXT("Charge"));
	FMiningHitContext Revoked; Mining->OnPrepareHit.Broadcast(Revoked);
	TestEqual(TEXT("Revoking releases damage hooks"), Revoked.DamageMultiplier, 1.f);
	TestTrue(TEXT("Revoking releases state"), Effects->GetActiveEffectViews().IsEmpty());
	Unit->Destroy();
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSharedCargoRecoveryTest, "MineLearning.Expansion.SharedCargoRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSharedCargoRecoveryTest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AHaulerCharacter* A = World->SpawnActor<AHaulerCharacter>(AHaulerCharacter::StaticClass(), FVector(0.f, -75.f, 100.f), FRotator::ZeroRotator, Params);
	AHaulerCharacter* B = World->SpawnActor<AHaulerCharacter>(AHaulerCharacter::StaticClass(), FVector(0.f, 75.f, 100.f), FRotator::ZeroRotator, Params);
	AHaulerAIController* FirstAI = World->SpawnActor<AHaulerAIController>(); FirstAI->Possess(A);
	AHaulerAIController* SecondAI = World->SpawnActor<AHaulerAIController>(); SecondAI->Possess(B);
	AActor* Destination = World->SpawnActor<AActor>();
	USceneComponent* Point = NewObject<USceneComponent>(Destination);
	Destination->AddInstanceComponent(Point); Destination->SetRootComponent(Point); Point->RegisterComponent();
	Destination->SetActorLocation(FVector(1500.f, 0.f, 100.f));
	UResourceStorageComponent* Storage = NewObject<UResourceStorageComponent>(Destination);
	Destination->AddInstanceComponent(Storage); Storage->RegisterComponent();
	AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, Params);
	TArray<TObjectPtr<UStaticMesh>> Meshes;
	Meshes.Add(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Pickup->InitializeItem({EItemType::IronOre, 12}, Meshes);
	Pickup->SetExplicitDeliveryTarget(Destination, Storage, Point);
	Pickup->SetRequiresHauler(true);
	USharedCarryDefinition* Config = NewObject<USharedCarryDefinition>();
	Config->TaskClass = ASharedCarryTask::StaticClass();
	ASharedCarryTask* Task = World->SpawnActor<ASharedCarryTask>();
	TestTrue(TEXT("Two workers can claim one paid shipment"), Task->Start(A, B, Pickup, Config));
	TestFalse(TEXT("Claimed worker cannot take a second job"), FirstAI->IsAvailableForCooperation());
	Task->Tick(0.1f);
	TestEqual(TEXT("Shared capacity exceeds separate 4 + 4"), Task->Cargo->GetCapacity(), 12);
	TestEqual(TEXT("Exactly one cargo owner after pickup"), Task->Cargo->GetCurrentItemCount(), 12);
	TestEqual(TEXT("First participant has no duplicate cargo"), A->GetResourceCarryComponent()->GetCurrentItemCount(), 0);
	TestEqual(TEXT("Second participant has no duplicate cargo"), B->GetResourceCarryComponent()->GetCurrentItemCount(), 0);
	A->Destroy();
	Task->Tick(0.1f);
	int32 Recovered = B->GetResourceCarryComponent()->GetCurrentItemCount();
	for (TActorIterator<AItemPickup> It(World); It; ++It)
	{
		if (!It->IsActorBeingDestroyed() && It->GetItemStack().ItemType == EItemType::IronOre) { Recovered += It->GetAmount(); }
	}
	TestEqual(TEXT("Worker loss preserves all cargo, without duplication"), Recovered, 12);
	TestNull(TEXT("Surviving worker released from shared claim"), SecondAI->GetCooperativeTask());
	B->Destroy(); FirstAI->Destroy(); SecondAI->Destroy(); Destination->Destroy();
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSharedCargoDeliveryTest, "MineLearning.Expansion.SharedCargoDelivery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSharedCargoDeliveryTest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AHaulerCharacter* A = World->SpawnActor<AHaulerCharacter>(AHaulerCharacter::StaticClass(), FVector(0.f, -75.f, 100.f), FRotator::ZeroRotator, Params);
	AHaulerCharacter* B = World->SpawnActor<AHaulerCharacter>(AHaulerCharacter::StaticClass(), FVector(0.f, 75.f, 100.f), FRotator::ZeroRotator, Params);
	AHaulerAIController* FirstAI = World->SpawnActor<AHaulerAIController>(); FirstAI->Possess(A);
	AHaulerAIController* SecondAI = World->SpawnActor<AHaulerAIController>(); SecondAI->Possess(B);
	AActor* Destination = World->SpawnActor<AActor>();
	USceneComponent* Point = NewObject<USceneComponent>(Destination);
	Destination->AddInstanceComponent(Point); Destination->SetRootComponent(Point); Point->RegisterComponent();
	Destination->SetActorLocation(FVector(1500.f, 0.f, 100.f));
	UResourceStorageComponent* Storage = NewObject<UResourceStorageComponent>(Destination);
	Destination->AddInstanceComponent(Storage); Storage->RegisterComponent();
	AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, Params);
	TArray<TObjectPtr<UStaticMesh>> Meshes;
	Meshes.Add(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Pickup->InitializeItem({EItemType::IronOre, 12}, Meshes);
	Pickup->SetExplicitDeliveryTarget(Destination, Storage, Point);
	Pickup->SetRequiresHauler(true);
	USharedCarryDefinition* Config = NewObject<USharedCarryDefinition>();
	Config->TaskClass = ASharedCarryTask::StaticClass();
	ASharedCarryTask* Task = World->SpawnActor<ASharedCarryTask>();
	TestTrue(TEXT("Two workers can claim one paid shipment"), Task->Start(A, B, Pickup, Config));
	TestFalse(TEXT("Claimed worker cannot take a second job"), FirstAI->IsAvailableForCooperation());
	Task->Tick(0.1f);
	TestEqual(TEXT("Shared capacity exceeds separate 4 + 4"), Task->Cargo->GetCapacity(), 12);
	TestEqual(TEXT("Exactly one cargo owner after pickup"), Task->Cargo->GetCurrentItemCount(), 12);
	TestEqual(TEXT("First participant has no duplicate cargo"), A->GetResourceCarryComponent()->GetCurrentItemCount(), 0);
	TestEqual(TEXT("Second participant has no duplicate cargo"), B->GetResourceCarryComponent()->GetCurrentItemCount(), 0);
	for (int32 Frame = 0; Frame < 150 && !Task->IsActorBeingDestroyed(); ++Frame) { Task->Tick(0.05f); }
	TestEqual(TEXT("Whole shipment delivered exactly once"), Storage->GetStoredItemAmount(EItemType::IronOre), 12);
	TestNull(TEXT("First worker released after delivery"), FirstAI->GetCooperativeTask());
	TestNull(TEXT("Second worker released after delivery"), SecondAI->GetCooperativeTask());
	TestTrue(TEXT("Presentation task retires after delivery"), Task->IsActorBeingDestroyed());
	A->Destroy();
	B->Destroy(); FirstAI->Destroy(); SecondAI->Destroy(); Destination->Destroy();
	return true;
}
#endif
