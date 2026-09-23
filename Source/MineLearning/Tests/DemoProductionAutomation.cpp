#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Animation/AnimInstance.h"
#include "MineLearning/MineLearningPlayerController.h"
#include "MineLearning/Demo/DemoRunComponent.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/AI/MiningCompanionAIController.h"
#include "MineLearning/AI/HaulerCharacter.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Mining/MiningToolComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/Mining/OreProcessorMachine.h"
#include "MineLearning/Mining/SellStation.h"

// Uses real skills, drops, reservations, machine animations and transfers. Only travel
// is shortened by positioning the player; this is not a navigation or pacing test.
class FDemoProductionDevices : public IAutomationLatentCommand
{
public:
 explicit FDemoProductionDevices(FAutomationTestBase* InTest, bool bInManual = false) : Test(InTest), bManual(bInManual) {}
 virtual bool Update() override
 {
  UWorld* World = nullptr;
  for (const FWorldContext& Context : GEngine->GetWorldContexts())
  {
   if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
  }
  if (!World) { Test->AddError(TEXT("Production PIE missing")); return true; }
  AMineLearningPlayerController* PC = Cast<AMineLearningPlayerController>(World->GetFirstPlayerController());
  UDemoRunComponent* Run = PC->GetDemoRun();
  if (Started < 0.f)
  {
   Started = World->GetTimeSeconds();
   Test->TestTrue(TEXT("Briefing highlights Start"), Run->IsRecommendedCommand(EDemoCommand::Start));
   PC->ExecuteDemoCommand(EDemoCommand::Start);
   Test->TestFalse(TEXT("Start button closes terminal"), PC->IsDemoTerminalOpen());
   PC->ToggleDemoTerminal();
   PC->ExecuteDemoCommand(EDemoCommand::BuyCarrier);
   Test->TestFalse(TEXT("Rejected purchase also closes terminal"), PC->IsDemoTerminalOpen());
   if (PC->IsDemoTerminalOpen()) { PC->ToggleDemoTerminal(); }
  }
  if (World->GetTimeSeconds() - Started > 120.f)
  {
   Test->AddError(FString::Printf(TEXT("Physical production stalled at step %d: %s"), Step, *Run->GetStatusText().ToString()));
   return true;
  }
  if (World->GetTimeSeconds() < NextAction) { return false; }
  NextAction = World->GetTimeSeconds() + 0.7f;
  AWarehouseDepot* Warehouse = First<AWarehouseDepot>(World);
  AOreProcessorMachine* Processor = First<AOreProcessorMachine>(World);
  ASellStation* Sell = First<ASellStation>(World);
  if (!Warehouse || !Processor || !Sell) { Test->AddError(TEXT("Production facility missing")); return true; }
  if (!bCheckedFacilities)
  {
   bCheckedFacilities = true;
   TArray<UStaticMeshComponent*> Meshes;
   Warehouse->GetComponents(Meshes);
   for (UStaticMeshComponent* Mesh : Meshes)
   {
    if (Mesh->GetFName() == TEXT("Door")) { Test->TestEqual(TEXT("Warehouse flap has no collision"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision); }
   }
   TArray<USplineComponent*> Splines;
   Processor->GetComponents(Splines);
   for (USplineComponent* Spline : Splines)
   {
    if (Spline->GetFName() == TEXT("OreFlowSpline_Input"))
    {
     Test->TestTrue(TEXT("Delivery point is beside visible input hopper"), FVector::Dist2D(Processor->GetDeliveryPointWorldTransform().GetLocation(), Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World)) < 250.f);
    }
   }
  }
  UResourceStorageComponent* Storage = Warehouse->GetStorageComponent();
  APawn* Pawn = PC->GetPawn();
  UResourceCarryComponent* Carry = Pawn->FindComponentByClass<UResourceCarryComponent>();
  if (Step == 0)
  {
   AMiningCompanionCharacter* Buddy = Cast<AMiningCompanionCharacter>(Pawn);
   if (!Buddy) { Test->AddError(TEXT("Start did not provide OreBuddy")); return true; }
   if (bManual && bSentDirectOre)
   {
    if (!Carry->IsEmpty()) { return false; }
    Test->TestEqual(TEXT("Manual ore bypasses warehouse inventory and orders"), Storage->GetStoredItemAmount(EItemType::IronOre), 0);
    Run->ExecuteCommand(EDemoCommand::Carrier);
    Step = 3;
   }
   else if (Storage->GetStoredItemAmount(EItemType::IronOre) >= 4)
   {
    Test->TestTrue(TEXT("Real mining deposits at least four raw ore"), Storage->GetStoredItemAmount(EItemType::IronOre) >= 4);
    Run->ExecuteCommand(EDemoCommand::ProcessFour);
    Test->TestTrue(TEXT("Pending first order guides player to Carrier button"), Run->IsRecommendedCommand(EDemoCommand::Carrier));
    Run->ExecuteCommand(EDemoCommand::Carrier);
    Advance();
   }
   else if (Carry->GetCurrentItemCount() >= 4)
   {
    Buddy->GetMiningToolComponent()->CancelMining();
    if (bManual)
    {
     if (FVector::Dist2D(Pawn->GetActorLocation(), Processor->GetDeliveryPointWorldTransform().GetLocation()) > 300.f)
     {
      Test->TestFalse(TEXT("Cannot deliver ore remotely"), Buddy->TryDeliverToNearbyMachine());
      Test->TestEqual(TEXT("Out of range keeps all carried ore"), Carry->GetCurrentItemCount(), 4);
     }
     Place(Pawn, Processor->GetDeliveryPointWorldTransform().GetLocation());
     bSentDirectOre = Buddy->TryDeliverToNearbyMachine();
     if (bSentDirectOre) { Test->TestFalse(TEXT("Repeated interaction cannot duplicate a pending deposit"), Buddy->TryDeliverToNearbyMachine()); }
    }
    else { Place(Pawn, Warehouse->GetDeliveryPointWorldTransform().GetLocation()); }
   }
   else if (AItemPickup* Drop = Pickup(World, Pawn, EItemType::IronOre, false))
   {
    Buddy->GetMiningToolComponent()->CancelMining();
    Place(Pawn, Drop->GetActorLocation() + FVector(60.f, 0.f, 0.f));
    Buddy->TryUsePickupSkill();
    if (!bCheckedPickupRate && Buddy->GetMesh()->GetAnimInstance()->GetCurrentActiveMontage())
    {
     bCheckedPickupRate = true;
     UAnimInstance* Anim = Buddy->GetMesh()->GetAnimInstance();
     Test->TestEqual(TEXT("Player R uses doubled initial montage rate"), Anim->Montage_GetPlayRate(Anim->GetCurrentActiveMontage()), 4.f);
    }
   }
   else if (AMineableOre* Ore = First<AMineableOre>(World))
   {
    Place(Pawn, Ore->GetActorLocation() + FVector(210.f, 0.f, 0.f));
    Buddy->TryUseMiningSkill();
   }
  }
  else
  {
   AHaulerCharacter* Hauler = Cast<AHaulerCharacter>(Pawn);
   if (!Hauler) { Test->AddError(TEXT("Carrier form missing")); return true; }
   if (Step == 1 && Collect(World, Hauler, EItemType::IronOre, true, 4)) { Advance(); }
   else if (Step == 2)
   {
    FVector GuidedPoint;
    Test->TestTrue(TEXT("Loaded Carrier receives location guidance"), Run->GetGuidanceDestination(GuidedPoint));
    Test->TestTrue(TEXT("Guide and transfer share input point"), GuidedPoint.Equals(Processor->GetDeliveryPointWorldTransform().GetLocation(), 1.f));
    Place(Pawn, Processor->GetDeliveryPointWorldTransform().GetLocation());
    Hauler->TryPlayerTransfer();
    if (Carry->IsEmpty()) { Advance(); }
   }
   else if (Step == 3 && Collect(World, Hauler, EItemType::IronIngot, false, 2)) { Advance(); }
   else if (Step == 4)
   {
    if (bManual)
    {
     Test->TestEqual(TEXT("Fresh ingots can be sold without a warehouse order"), Storage->GetStoredItemAmount(EItemType::IronIngot), 0);
     Place(Pawn, Sell->GetRobotApproachPoint()->GetComponentLocation());
     Hauler->TryPlayerTransfer();
     if (Carry->IsEmpty()) { Step = 7; }
     return false;
    }
    Test->TestTrue(TEXT("Two output ingots correctly name the warehouse destination"), Hauler->GetPlayerCargoDescription().ToString().Contains(TEXT("仓库入口")));
    Place(Pawn, Warehouse->GetDeliveryPointWorldTransform().GetLocation());
    Hauler->TryPlayerTransfer();
    if (Storage->GetStoredItemAmount(EItemType::IronIngot) == 2)
    {
     Test->TestEqual(TEXT("Four mined ore become exactly two warehouse ingots"), Storage->GetStoredItemAmount(EItemType::IronOre), 0);
     Run->ExecuteCommand(EDemoCommand::SellTwo);
     Advance();
    }
   }
   else if (Step == 5 && Collect(World, Hauler, EItemType::IronIngot, true, 2)) { Advance(); }
   else if (Step == 6)
   {
    Place(Pawn, Sell->GetRobotApproachPoint()->GetComponentLocation());
    Hauler->TryPlayerTransfer();
    if (Carry->IsEmpty()) { Advance(); }
   }
   else if (Step == 7 && Collect(World, Hauler, EItemType::Coin, false, 4)) { Advance(); }
   else if (Step == 8)
   {
    Place(Pawn, Warehouse->GetDeliveryPointWorldTransform().GetLocation());
    Hauler->TryPlayerTransfer();
    if (Storage->GetStoredItemAmount(EItemType::Coin) == 4)
    {
     Test->TestTrue(TEXT("First Carrier purchased only with physically earned coins"), Run->ExecuteCommand(EDemoCommand::BuyCarrier));
     Test->TestEqual(TEXT("Purchase consumes all four earned coins"), Storage->GetStoredItemAmount(EItemType::Coin), 0);
     Test->TestTrue(TEXT("Real pickup montage rate was observed"), bCheckedPickupRate);
     for (TActorIterator<APawn> It(World); It; ++It)
     {
      TInlineComponentArray<UPrimitiveComponent*> Primitives(*It);
      for (UPrimitiveComponent* Primitive : Primitives)
      {
       Test->TestEqual(TEXT("Placed and newly spawned pawns ignore camera probes"), Primitive->GetCollisionResponseToChannel(ECC_Camera), ECR_Ignore);
      }
     }
     Test->TestTrue(TEXT("Spring arm still tests environment obstruction"), Hauler->FindComponentByClass<USpringArmComponent>()->bDoCollisionTest);
     Test->AddInfo(TEXT("Zero stock -> live mining -> manual Carrier -> processor 2:1 -> sale 1:2 -> earned robot purchase passed."));
     if (!bManual) { return true; }
     UClass* BuddyClass = LoadClass<AMiningCompanionCharacter>(nullptr, TEXT("/Game/MineLearning/Characters/OreBuddy/Blueprints/BP_OreBuddy07.BP_OreBuddy07_C"));
     FActorSpawnParameters Spawn;
     Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
     AIBuddy = World->SpawnActor<AMiningCompanionCharacter>(BuddyClass, Pawn->GetActorLocation(), FRotator::ZeroRotator, Spawn);
     if (!AIBuddy.IsValid()) { Test->AddError(TEXT("AI pickup fixture failed to spawn")); return true; }
     AMineableOre* Ore = First<AMineableOre>(World);
     Place(AIBuddy.Get(), Ore->GetActorLocation() + FVector(250.f, 0.f, 0.f));
     AIBuddy->SpawnDefaultController();
     Step = 9;
    }
   }
   else if (Step == 9 && AIBuddy.IsValid())
   {
    AMiningCompanionAIController* AI = Cast<AMiningCompanionAIController>(AIBuddy->GetController());
    UAnimInstance* Anim = AIBuddy->GetMesh()->GetAnimInstance();
    if (AI && Anim && Anim->GetCurrentActiveMontage() == AI->GetCollectMontage())
    {
     Test->TestEqual(TEXT("AI uses the same accelerated pickup montage rate"), Anim->Montage_GetPlayRate(AI->GetCollectMontage()), 4.f);
     bCheckedAIPickupRate = true;
    }
    if (bCheckedAIPickupRate && AIBuddy->GetResourceCarryComponent()->GetCurrentItemCount() > 0)
    {
     Test->AddInfo(TEXT("AI played accelerated pickup and collected ore through animation notify."));
     return true;
    }
   }
  }
  return false;
 }
private:
 template<typename T> T* First(UWorld* World) const
 {
  for (TActorIterator<T> It(World); It; ++It) { if (IsValid(*It)) { return *It; } }
  return nullptr;
 }
 AItemPickup* Pickup(UWorld* World, APawn* Pawn, EItemType Type, bool bOrder) const
 {
  for (TActorIterator<AItemPickup> It(World); It; ++It)
  {
   if (It->GetItemStack().ItemType == Type && It->IsAvailableFor(Pawn)
    && (IsValid(It->GetReservationSourceStorage()) == bOrder)) { return *It; }
  }
  return nullptr;
 }
 bool Collect(UWorld* World, AHaulerCharacter* Pawn, EItemType Type, bool bOrder, int32 Count)
 {
  if (Pawn->GetResourceCarryComponent()->GetCurrentItemCount() >= Count) { return true; }
  if (AItemPickup* Item = Pickup(World, Pawn, Type, bOrder))
  {
   Place(Pawn, Item->GetActorLocation() + FVector(80.f, 0.f, 0.f));
   Pawn->TryPlayerTransfer();
  }
  return false;
 }
 void Place(APawn* Pawn, FVector Point)
 {
  if (FVector::Dist2D(Pawn->GetActorLocation(), Point) < 35.f) { return; }
  FHitResult Hit;
  FCollisionQueryParams Query(SCENE_QUERY_STAT(DemoDeviceTest), false, Pawn);
  const FVector Start(Point.X, Point.Y, 1800.f);
  const FVector End(Point.X, Point.Y, -500.f);
  if (Pawn->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Query)) { Point.Z = Hit.ImpactPoint.Z; }
  Point.Z += Pawn->FindComponentByClass<UCapsuleComponent>()->GetScaledCapsuleHalfHeight() + 2.f;
  Pawn->SetActorLocation(Point, false, nullptr, ETeleportType::TeleportPhysics);
 }
 void Advance() { ++Step; UE_LOG(LogTemp, Display, TEXT("[DemoDeviceTest] Step=%d"), Step); }
 FAutomationTestBase* Test;
 int32 Step = 0;
 float Started = -1.f;
 float NextAction = 0.f;
 bool bCheckedFacilities = false;
 bool bCheckedPickupRate = false;
 bool bManual = false;
 bool bSentDirectOre = false;
 bool bCheckedAIPickupRate = false;
 TWeakObjectPtr<AMiningCompanionCharacter> AIBuddy;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemoProductionTest, "MineLearning.Demo.PhysicalProduction",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemoProductionTest::RunTest(const FString& Parameters)
{
 ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
 ADD_LATENT_AUTOMATION_COMMAND(FDemoProductionDevices(this));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemoManualProductionTest, "MineLearning.Demo.ManualProduction",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemoManualProductionTest::RunTest(const FString& Parameters)
{
 ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
 ADD_LATENT_AUTOMATION_COMMAND(FDemoProductionDevices(this, true));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
 return true;
}
#endif
