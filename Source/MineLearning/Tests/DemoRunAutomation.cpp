#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MineLearning/MineLearningPlayerController.h"
#include "MineLearning/Demo/DemoRunComponent.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/AI/HaulerCharacter.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/Manifestation/Guren/GurenQSkillComponent.h"
#include "MineLearning/Manifestation/Guren/QGrabTestDummy.h"
#include "Components/CapsuleComponent.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/TextBlock.h"

#include "MineLearning/Combat/CombatDamageSubsystem.h"
#include "Components/StaticMeshComponent.h"
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FDemoNavigationCheck, FAutomationTestBase*, Test);
bool FDemoNavigationCheck::Update()
{
 UWorld* World = nullptr;
 for (const FWorldContext& Context : GEngine->GetWorldContexts())
 {
  if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
 }
 if (!World) { Test->AddError(TEXT("Navigation PIE missing")); return true; }
 int32 OreCount = 0;
 for (TActorIterator<AMineableOre> It(World); It; ++It)
 {
  ++OreCount;
  Test->TestTrue(TEXT("Ramp exit contains no generated ore"), FVector::Dist2D(It->GetActorLocation(), FVector(-419,-451,28)) > 150.f);
 }
 Test->TestEqual(TEXT("Five authored ore slots remain"), OreCount, 5);
 UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
 if (!Test->TestNotNull(TEXT("Mining map navigation system"), Nav)) { return true; }
 if (UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(World))
 {
  if (World->GetTimeSeconds() > 30.f) { Test->AddError(TEXT("Navigation did not become ready")); return true; }
  return false;
 }
 TArray<FVector> Points = { FVector(972,1975,262), FVector(-839,-451,28), FVector(-436,-210,123), FVector(0,0,250), FVector(626,1076,364) };
 for (int32 Index = 0; Index < Points.Num(); ++Index)
 {
  FNavLocation Projected;
  const bool bFound = Nav->ProjectPointToNavigation(Points[Index], Projected, FVector(250,250,500));
  Test->AddInfo(FString::Printf(TEXT("NavPoint %d Found=%d Input=%s Result=%s"), Index, bFound, *Points[Index].ToString(), *Projected.Location.ToString()));
  if (bFound) { Points[Index] = Projected.Location; }
  Test->TestTrue(*FString::Printf(TEXT("Point %d has walkable navigation"), Index), bFound);
 }
 for (int32 Index = 0; Index < Points.Num()-1; ++Index)
 {
  UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Points[Index], Points.Last());
  Test->TestTrue(*FString::Printf(TEXT("Point %d has a complete path to warehouse"), Index), Path && Path->IsValid() && !Path->IsPartial());
  Test->AddInfo(FString::Printf(TEXT("NavPath %d Valid=%d Partial=%d Nodes=%d"), Index, Path && Path->IsValid(), Path && Path->IsPartial(), Path ? Path->PathPoints.Num() : 0));
 }
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemoNavigationTest, "MineLearning.Demo.NavigationContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemoNavigationTest::RunTest(const FString& Parameters)
{
 ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(5.f));
 ADD_LATENT_AUTOMATION_COMMAND(FDemoNavigationCheck(this));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
 return true;
}

// Transactions and retries use explicit stock fixtures; BossLiveCombat below uses
// real shot resolution, authored damage, reload notifies and full boss health.
class FDemoBossEconomyCheck : public IAutomationLatentCommand
{
public:
 explicit FDemoBossEconomyCheck(FAutomationTestBase* InTest) : Test(InTest) {}
 bool Update() override
 {
  UWorld* World = nullptr;
  for (const FWorldContext& C : GEngine->GetWorldContexts()) { if (C.WorldType == EWorldType::PIE) { World = C.World(); break; } }
  if (!World) { Test->AddError(TEXT("Boss economy PIE missing")); return true; }
  auto* PC = Cast<AMineLearningPlayerController>(World->GetFirstPlayerController());
  auto* Run = PC->GetDemoRun();
  AWarehouseDepot* Warehouse = *TActorIterator<AWarehouseDepot>(World);
  auto* Storage = Warehouse->GetStorageComponent();
  const float Now = World->GetTimeSeconds();
  if (Step == 0)
  {
   Test->TestTrue(TEXT("Start unlimited production"), Run->ExecuteCommand(EDemoCommand::Start));
   Test->TestEqual(TEXT("No production countdown"), Run->RemainingSeconds, 0.f);
   Storage->AddItem({EItemType::Coin,60}); Storage->AddItem({EItemType::IronIngot,40});
   Test->TestTrue(TEXT("First upgrade"),Run->ExecuteCommand(EDemoCommand::UpgradeStrength));
   Test->TestTrue(TEXT("Second upgrade"),Run->ExecuteCommand(EDemoCommand::UpgradeStrength));
   Test->TestTrue(TEXT("Third upgrade"),Run->ExecuteCommand(EDemoCommand::UpgradeStrength));
   Test->TestFalse(TEXT("Upgrade capped"),Run->ExecuteCommand(EDemoCommand::UpgradeStrength));
   Test->TestEqual(TEXT("Escalating upgrade total price 12"), Storage->GetAvailableItemAmount(EItemType::Coin),48);
   Test->TestTrue(TEXT("Unlock Gunner"),Run->ExecuteCommand(EDemoCommand::UnlockGunner));
   Test->TestTrue(TEXT("Transform Gunner"),Run->ExecuteCommand(EDemoCommand::Gunner));
   auto* Gunner = Cast<AGunnerCharacter>(PC->GetPawn());
   if (!Test->TestNotNull(TEXT("Player Gunner"),Gunner)) { return true; }
   Test->TestEqual(TEXT("No free initial magazine"),Gunner->GetCurrentAmmo(),0);
   Test->TestFalse(TEXT("Cannot reload without purchased magazine"),Gunner->RequestReload());
   Test->TestTrue(TEXT("Buy magazine"),Run->ExecuteCommand(EDemoCommand::BuyMagazine));
   Test->TestEqual(TEXT("One ingot per magazine"),Storage->GetAvailableItemAmount(EItemType::IronIngot),37);
   Test->TestTrue(TEXT("Paid reload starts"),Gunner->RequestReload());
   Test->TestEqual(TEXT("Reserve spent once"),Run->GetReserveMagazines(),0);
   Test->TestFalse(TEXT("No duplicate reload"),Gunner->RequestReload());
   Test->TestFalse(TEXT("Cannot transform during reload"),Run->ExecuteCommand(EDemoCommand::OreBuddy));
   WaitUntil=Now+12;Step=1;return false;
  }
  if (Step==1)
  {
   auto* Gunner=Cast<AGunnerCharacter>(PC->GetPawn());
   if(Gunner->IsWeaponBusy()) { if(Now>WaitUntil){Test->AddError(TEXT("Reload stuck"));return true;} return false; }
   Test->TestEqual(TEXT("Real reload provides twenty rounds"),Gunner->GetCurrentAmmo(),20);
   Gunner->RestoreLoadedAmmo(7);Run->RecordGunnerAmmo(7);
   Test->TestTrue(TEXT("Leave Gunner"),Run->ExecuteCommand(EDemoCommand::OreBuddy));
   Test->TestTrue(TEXT("Return Gunner"),Run->ExecuteCommand(EDemoCommand::Gunner));
   Test->TestEqual(TEXT("Transform cannot refill ammo"),Cast<AGunnerCharacter>(PC->GetPawn())->GetCurrentAmmo(),7);
   Storage->TryReserveItem({EItemType::Coin,40});
   Test->TestFalse(TEXT("Reserved funds cannot summon"),Run->ExecuteCommand(EDemoCommand::SubmitMaterials));
   Test->TestEqual(TEXT("Rejected summon atomic ingots"),Storage->GetAvailableItemAmount(EItemType::IronIngot),37);
   Storage->ReleaseReservedItem({EItemType::Coin,40});
   Run->ChallengeDuration=2;
   Test->TestTrue(TEXT("Summon without robot prerequisites"),Run->ExecuteCommand(EDemoCommand::SubmitMaterials));
   Test->TestNotNull(TEXT("Boss spawned"),Run->BossTarget.Get());
   Test->TestEqual(TEXT("Authored boss health"),Run->BossTarget->GetMaxHealth(),8000.f);
   Test->TestEqual(TEXT("Summon coins"),Storage->GetAvailableItemAmount(EItemType::Coin),28);
   Test->TestEqual(TEXT("Summon ingots"),Storage->GetAvailableItemAmount(EItemType::IronIngot),25);
   Test->TestFalse(TEXT("No duplicate active summon"),Run->ExecuteCommand(EDemoCommand::SubmitMaterials));
   WaitUntil=Now+3;Step=2;return false;
  }
  if(Step==2)
  {
   if(Now<WaitUntil) return false;
   Test->TestEqual(TEXT("Boss timeout resumes production"),Run->GetPhase(),EDemoPhase::Production);
   Test->TestFalse(TEXT("Timeout is not game over"),Run->IsFinished());
   Test->TestNull(TEXT("Expired target removed"),Run->BossTarget.Get());
   Test->TestEqual(TEXT("Core survives timeout"),Run->CoreBonus.Strength,15.f);
   Test->TestEqual(TEXT("Ammo survives timeout"),Run->GetLoadedGunnerAmmo(),7);
   Run->ChallengeDuration=240;
   Test->TestTrue(TEXT("Paid retry"),Run->ExecuteCommand(EDemoCommand::SubmitMaterials));
   Test->TestEqual(TEXT("Retry charged again"),Storage->GetAvailableItemAmount(EItemType::Coin),8);
   Test->TestEqual(TEXT("Two attempts"),Run->BossAttempts,2);
   World->GetSubsystem<UCombatDamageSubsystem>()->ApplyDebugDamage(Run->BossTarget,8000);
   Test->TestEqual(TEXT("Only boss death wins"),Run->GetPhase(),EDemoPhase::Victory);
   Test->TestTrue(TEXT("Production remains available after win"),Run->ExecuteCommand(EDemoCommand::BuyCarrier));
   return true;
  }
  return true;
 }
private:
 FAutomationTestBase* Test;
 int32 Step=0;
 float WaitUntil=0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemoBossEconomyTest,"MineLearning.Demo.BossEconomyContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDemoBossEconomyTest::RunTest(const FString& Parameters)
{
 ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
 ADD_LATENT_AUTOMATION_COMMAND(FDemoBossEconomyCheck(this));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
 return true;
}
class FDemoBossCombatCheck : public IAutomationLatentCommand
{
public:
 explicit FDemoBossCombatCheck(FAutomationTestBase* InTest):Test(InTest){}
 bool Update() override
 {
  UWorld* World=nullptr;
  for(const FWorldContext& C:GEngine->GetWorldContexts()) if(C.WorldType==EWorldType::PIE){World=C.World();break;}
  if(!World){Test->AddError(TEXT("Live boss PIE missing"));return true;}
  auto* PC=Cast<AMineLearningPlayerController>(World->GetFirstPlayerController());
  auto* Run=PC->GetDemoRun();
  const float Now=World->GetTimeSeconds();
  if(Started<0)
  {
   Run->ExecuteCommand(EDemoCommand::Start);
   auto* Storage=(*TActorIterator<AWarehouseDepot>(World))->GetStorageComponent();
   Storage->AddItem({EItemType::Coin,20});Storage->AddItem({EItemType::IronIngot,28});
   Run->ExecuteCommand(EDemoCommand::UnlockGunner);
   for(int32 I=0;I<14;++I)Run->ExecuteCommand(EDemoCommand::BuyMagazine);
   Run->ExecuteCommand(EDemoCommand::Gunner);
   if(PC->IsDemoTerminalOpen())PC->ToggleDemoTerminal();
   Test->TestTrue(TEXT("Real boss summon"),Run->ExecuteCommand(EDemoCommand::SubmitMaterials));
   if(!Run->BossTarget)return true;
   PC->GetPawn()->SetActorLocation(Run->BossTarget->GetActorLocation()+FVector(-650,0,160));
   Started=Now;
  }
  if(Run->GetPhase()==EDemoPhase::Victory)
  {
   Test->AddInfo(FString::Printf(TEXT("Full 8000 HP boss defeated with real Gunner shots in %.1fs; loaded=%d reserves=%d"),Now-Started,Run->GetLoadedGunnerAmmo(),Run->GetReserveMagazines()));
   Test->TestTrue(TEXT("Boss is sustained combat"),Now-Started>30.f);
   return true;
  }
  if(Run->GetPhase()!=EDemoPhase::BossChallenge||Now-Started>245.f){Test->AddError(TEXT("Baseline Gunner did not defeat full boss before deadline"));return true;}
  if(Now<NextShot)return false;
  NextShot=Now+.12f;
  auto* Gunner=Cast<AGunnerCharacter>(PC->GetPawn());
  if(!Gunner||!Run->BossTarget){Test->AddError(TEXT("Missing combat actors"));return true;}
  if(Gunner->GetCurrentAmmo()==0)
  {
   if(!Gunner->IsWeaponBusy()&&Run->GetReserveMagazines()==0){Test->AddError(TEXT("Fourteen magazines insufficient"));return true;}
   Gunner->RequestReload();
  }
  else
  {
   FVector Origin,Extent;Run->BossTarget->GetActorBounds(true,Origin,Extent);
   Gunner->SetActorRotation((Origin-Gunner->GetActorLocation()).Rotation());
   Gunner->TryFireAtAim(Gunner->GetMuzzleLocation(),(Origin-Gunner->GetMuzzleLocation()).GetSafeNormal());
  }
  return false;
 }
private:
 FAutomationTestBase* Test;
 float Started=-1,NextShot=0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemoBossCombatTest,"MineLearning.Demo.BossLiveCombat",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDemoBossCombatTest::RunTest(const FString& Parameters)
{
 ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
 ADD_LATENT_AUTOMATION_COMMAND(FDemoBossCombatCheck(this));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
 return true;
}
#endif
