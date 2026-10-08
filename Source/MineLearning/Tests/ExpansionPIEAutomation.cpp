#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "MineLearning/MineLearningPlayerController.h"
#include "MineLearning/Demo/DemoRunComponent.h"
#include "MineLearning/Roguelite/MineRunCoordinatorComponent.h"
#include "MineLearning/Roguelite/MetaProgressComponent.h"
#include "MineLearning/Roguelite/RunBuildComponent.h"
#include "MineLearning/Roguelite/RunAbilityComponent.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/AI/HaulerCharacter.h"
#include "MineLearning/AI/HaulerAIController.h"
#include "MineLearning/AI/CarrierAnimInstance.h"
#include "Camera/CameraActor.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/AI/SharedCarryTask.h"
#include "MineLearning/AI/CooperativeHaulingComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

class FExpansionPIECheck : public IAutomationLatentCommand
{
public:
	explicit FExpansionPIECheck(FAutomationTestBase* InTest) : Test(InTest) {}
	bool Update() override
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
		}
		AMineLearningPlayerController* Player = World ? Cast<AMineLearningPlayerController>(World->GetFirstPlayerController()) : nullptr;
		if (!Player) { Test->AddError(TEXT("Expansion PIE player missing")); return true; }
		UMineRunCoordinatorComponent* Coordinator = Player->GetRunCoordinator();
		UDemoRunComponent* Run = Player->GetDemoRun();
		URunAbilityComponent* Ability = Player->FindComponentByClass<URunAbilityComponent>();
		if (!Ability) { Test->AddError(TEXT("Run ability assembly missing")); return true; }
		const float Now = World->GetTimeSeconds();
		if (Now < Until) { return false; }
		switch (Step)
		{
		case 0:
		{
			URunContentCatalog* Catalog = DuplicateObject(Coordinator->GetCatalog(), GetTransientPackage());
			Catalog->SaveSlot = TEXT("Automation_Expansion_") + FGuid::NewGuid().ToString();
			Coordinator->GetMetaProgress()->Initialize(Catalog);
			Coordinator->GetMetaProgress()->AddDebugPoints(20);
			for (FName Id : {FName("Overclocker"), FName("CoworkerProgramming"), FName("MiningMods"), FName("LogisticsMods")})
			{
				Test->TestTrue(TEXT("Research new permanent node"), Coordinator->Research(Id));
			}
			Test->TestTrue(TEXT("Select Overclocker"), Coordinator->SelectSummoner(TEXT("Overclocker")));
			Test->TestTrue(TEXT("Start Overclocker run"), Run->ExecuteCommand(EDemoCommand::Start));
			Player->CloseRogueliteMenu();
			Test->TestFalse(TEXT("Identity grants no free active ability"), Ability->ActivateOverclock());
			Test->TestTrue(TEXT("Acquire authored active card"), Coordinator->GrantDebugUpgrade(TEXT("Overclock")));
			Test->TestTrue(TEXT("Enter OreBuddy"), Run->ExecuteCommand(EDemoCommand::OreBuddy));
			Test->TestTrue(TEXT("Activate Overclock"), Ability->ActivateOverclock());
			ActivationTime = Now;
			Test->TestEqual(TEXT("Authored attack boost"), Player->GetPawn()->FindComponentByClass<UCombatComponent>()->GetAttackSpeedScale(), 2.f);
			CheckStatus(World, TEXT("超频中"));
			Until = Now + 2.f; ++Step; return false;
		}
		case 1:
		{
			TWeakObjectPtr<APawn> Previous = Player->GetPawn();
			Test->TestTrue(TEXT("Swap to Carrier during boost"), Run->ExecuteCommand(EDemoCommand::Carrier));
			Test->TestTrue(TEXT("Pawn swap cannot restart cooldown"), Ability->GetCooldownRemaining() < 18.1f);
			Test->TestEqual(TEXT("Boost transferred to new form"), Player->GetPawn()->FindComponentByClass<UCombatComponent>()->GetCastSpeedScale(), 2.f);
			if (Previous.IsValid()) { Test->TestEqual(TEXT("Old pawn loses boost"), Previous->FindComponentByClass<UCombatComponent>()->GetAttackSpeedScale(), 1.f); }
			Until = ActivationTime + 6.15f; ++Step; return false;
		}
		case 2:
			Test->TestEqual(TEXT("Six seconds enters overheat"), Ability->GetPhase(), EOverclockPhase::Overheat);
			Test->TestEqual(TEXT("Overheat slows current form"), Player->GetPawn()->FindComponentByClass<UCombatComponent>()->GetMoveSpeedScale(), 0.8f);
			CheckStatus(World, TEXT("过热"));
			Until = ActivationTime + 9.15f; ++Step; return false;
		case 3:
			Test->TestEqual(TEXT("Three seconds later only cooldown remains"), Ability->GetPhase(), EOverclockPhase::Cooldown);
			Test->TestEqual(TEXT("Overheat modifier expires"), Player->GetPawn()->FindComponentByClass<UCombatComponent>()->GetMoveSpeedScale(), 1.f);
			Test->TestFalse(TEXT("Cooldown cannot be bypassed"), Ability->ActivateOverclock());
			Until = ActivationTime + 20.15f; ++Step; return false;
		case 4:
			Test->TestEqual(TEXT("Authored twenty-second cooldown"), Ability->GetPhase(), EOverclockPhase::Ready);
			Test->TestTrue(TEXT("Clean Overclocker test save"), Coordinator->GetMetaProgress()->ClearProfile());
			Test->TestTrue(TEXT("Restart returns preparation"), Run->ExecuteCommand(EDemoCommand::Restart));
			Until = 0.f; Step = 40; return false;
		case 40:
			// OpenLevel is asynchronous and replaces the old world and its components.
			if (Run->GetPhase() != EDemoPhase::Briefing) { return false; }
			{
				URunContentCatalog* Catalog = DuplicateObject(Coordinator->GetCatalog(), GetTransientPackage());
				Catalog->SaveSlot = TEXT("Automation_Expansion_") + FGuid::NewGuid().ToString();
				Coordinator->GetMetaProgress()->Initialize(Catalog);
				Coordinator->GetMetaProgress()->AddDebugPoints(10);
				Test->TestTrue(TEXT("Research Coworker on fresh isolated world"), Coordinator->Research(TEXT("CoworkerProgramming")));
				Test->TestTrue(TEXT("Research logistics on fresh isolated world"), Coordinator->Research(TEXT("LogisticsMods")));
			}
			Test->TestTrue(TEXT("Select Coworker"), Coordinator->SelectSummoner(TEXT("CoworkerProgramming")));
			Test->TestTrue(TEXT("Start Coworker"), Run->ExecuteCommand(EDemoCommand::Start));
			Player->CloseRogueliteMenu();
			Test->TestEqual(TEXT("Production starter miner"), Run->PurchasedOreBuddies, 1);
			Test->TestEqual(TEXT("Production starter carrier"), Run->PurchasedCarriers, 1);
			Test->TestFalse(TEXT("Identity blocks player transformation"), Run->ExecuteCommand(EDemoCommand::OreBuddy));
			Test->TestNull(TEXT("No free main-thread focus"), Ability->GetFocusUnit());
			Test->TestTrue(TEXT("Acquire MainThread"), Coordinator->GrantDebugUpgrade(TEXT("MainThread")));
			Until = Now + 0.25f; Step = 5; return false;
		case 5:
		{
			APawn* First = Ability->GetFocusUnit();
			Test->TestNotNull(TEXT("Initial focus chosen"), First);
			CheckWork(First, 1.f);
			Test->TestTrue(TEXT("X ability cycles stable worker list"), Ability->CycleFocus());
			APawn* Second = Ability->GetFocusUnit();
			Test->TestTrue(TEXT("Focus changes to other worker"), Second && Second != First);
			CheckWork(First, 0.25f); CheckWork(Second, 1.f);
			CheckStatus(World, TEXT("[X]"));
			if (Second) { Second->Destroy(); }
			Test->TestEqual(TEXT("Destroyed focus transfers automatically"), Ability->GetFocusUnit(), First);
			CheckWork(First, 1.f);
			const ACharacter* AI = Cast<ACharacter>(First);
			const FVector BeforeScale = AI ? AI->GetMesh()->GetRelativeScale3D() : FVector::OneVector;
			Test->TestTrue(TEXT("Acquire player-only scale card"), Coordinator->GrantDebugUpgrade(TEXT("GiantSlayerCarrier")));
			if (AI) { Test->TestEqual(TEXT("Generic size card leaves AI scale unchanged"), AI->GetMesh()->GetRelativeScale3D(), BeforeScale); }
			Coordinator->GrantDebugResources();
			Test->TestTrue(TEXT("New AI hire"), Run->ExecuteCommand(EDemoCommand::BuyCarrier));
			for (TActorIterator<AHaulerCharacter> It(World); It; ++It)
			{
				if (!It->IsActorBeingDestroyed() && *It != First && It->FindComponentByClass<UUnitEffectComponent>()) { CheckWork(*It, 0.25f); }
			}
			Test->TestTrue(TEXT("Acquire shared hauling"), Coordinator->GrantDebugUpgrade(TEXT("CoopHeavyCarry")));
			Test->TestEqual(TEXT("New shipments support real large cargo"), Run->GetWarehouse()->GetDispatchBatchSize(), 12);
			Test->TestTrue(TEXT("Clean isolated save"), Coordinator->GetMetaProgress()->ClearProfile());
			return true;
		}
		}
		return true;
	}
private:
	void CheckWork(APawn* Unit, float Power)
	{
		if (!Unit) { return; }
		if (AHaulerCharacter* Carrier = Cast<AHaulerCharacter>(Unit))
		{
			Test->TestEqual(TEXT("AI real carry output"), Carrier->GetResourceCarryComponent()->GetCapacity(), FMath::CeilToInt(4.f * (1.f + Power)));
		}
		else { Test->TestEqual(TEXT("AI primary damage output"), Unit->FindComponentByClass<UCombatComponent>()->GetModifiers().PrimaryDamage, Power); }
		Test->TestEqual(TEXT("Work power never adds speed"), Unit->FindComponentByClass<UCombatComponent>()->GetAttackSpeedScale(), 1.f);
	}
	void CheckStatus(UWorld* World, const TCHAR* Text)
	{
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UUserWidget::StaticClass(), true);
		bool bFound = false;
		for (UUserWidget* Widget : Widgets)
		{
			UTextBlock* Label = Widget->WidgetTree ? Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("AbilityStatus"))) : nullptr;
			bFound |= Label && Label->GetText().ToString().Contains(Text);
		}
		Test->TestTrue(TEXT("UMG receives ability state event"), bFound);
	}
	FAutomationTestBase* Test;
	int32 Step = 0;
	float Until = 0.f;
	float ActivationTime = 0.f;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpansionPIETest, "MineLearning.Expansion.PIEAbilities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpansionPIETest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FExpansionPIECheck(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

class FSharedCarryPIECheck : public IAutomationLatentCommand
{
public:
	explicit FSharedCarryPIECheck(FAutomationTestBase* InTest) : Test(InTest) {}
	bool Update() override
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts()) { if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; } }
		AMineLearningPlayerController* Player = World ? Cast<AMineLearningPlayerController>(World->GetFirstPlayerController()) : nullptr;
		if (!Player) { Test->AddError(TEXT("Hauling PIE player missing")); return true; }
		if (Step == 0)
		{
			Player->CloseRogueliteMenu();
			FActorSpawnParameters Params; Params.Owner = Player; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			A = World->SpawnActor<AHaulerCharacter>(AHaulerCharacter::StaticClass(), FVector(75, 500, 314), FRotator(0, 90, 0), Params);
			B = World->SpawnActor<AHaulerCharacter>(AHaulerCharacter::StaticClass(), FVector(-75, 500, 314), FRotator(0, 90, 0), Params);
			A->SpawnDefaultController(); B->SpawnDefaultController();
			AActor* Destination = World->SpawnActor<AActor>();
			USceneComponent* Point = NewObject<USceneComponent>(Destination);
			Destination->AddInstanceComponent(Point); Destination->SetRootComponent(Point); Point->RegisterComponent();
			Destination->SetActorLocation(FVector(0, 1250, 314));
			Storage = NewObject<UResourceStorageComponent>(Destination);
			Destination->AddInstanceComponent(Storage.Get()); Storage->RegisterComponent();
			AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), FVector(0, 500, 314), FRotator::ZeroRotator, Params);
			TArray<TObjectPtr<UStaticMesh>> Meshes = {LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"))};
			Pickup->InitializeItem({EItemType::IronOre, 12}, Meshes);
			Pickup->ReleaseStationaryForCollection(); Pickup->SetExplicitDeliveryTarget(Destination, Storage.Get(), Point); Pickup->SetRequiresHauler(true);
			USharedCarryDefinition* Definition = LoadObject<USharedCarryDefinition>(nullptr, TEXT("/Game/MineLearning/GameplayRuntime/Effects/DA_CoopHeavyCarry.DA_CoopHeavyCarry"));
			UCooperativeHaulingComponent* Hauling = Player->FindComponentByClass<UCooperativeHaulingComponent>();
			if (!Test->TestNotNull(TEXT("Run owns hauling dispatcher"), Hauling)) { return true; }
			Player->GetRunCoordinator()->AssembleUnit(A.Get()); Player->GetRunCoordinator()->AssembleUnit(B.Get());
			Hauling->Configure(nullptr, Definition);
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			FNavLocation StartNav, EndNav;
			const bool bStartNav = Nav && Nav->ProjectPointToNavigation(Pickup->GetActorLocation(), StartNav, FVector(100, 100, 300));
			const bool bEndNav = Nav && Nav->ProjectPointToNavigation(Destination->GetActorLocation(), EndNav, FVector(100, 100, 300));
			UNavigationPath* Path = bStartNav && bEndNav ? UNavigationSystemV1::FindPathToLocationSynchronously(World, StartNav.Location, EndNav.Location, A.Get()) : nullptr;
			Test->AddInfo(FString::Printf(TEXT("Pair navigation: start=%d end=%d valid=%d partial=%d"), bStartNav, bEndNav, Path && Path->IsValid(), Path && Path->IsPartial()));
			Test->TestTrue(TEXT("Authored pair configuration valid"), Definition && Definition->IsValidConfiguration());
			Camera = World->SpawnActor<ACameraActor>();
			Camera->SetActorLocation(FVector(500, 1050, 580));
			Camera->SetActorRotation((FVector(0, 820, 310) - Camera->GetActorLocation()).Rotation());
			Player->SetViewTarget(Camera.Get());
			Started = World->GetTimeSeconds(); Step = 1; return false;
		}
		const float Elapsed = World->GetTimeSeconds() - Started;
		if (Step == 2)
		{
			if (Elapsed < 0.25f) { return false; }
			Test->TestEqual(TEXT("Relay takes real player cargo"), A->GetResourceCarryComponent()->GetCurrentItemCount(), 0);
			Test->TestEqual(TEXT("Relay receiver owns all transferred cargo"), B->GetResourceCarryComponent()->GetCurrentItemCount(), 3);
			Test->TestEqual(TEXT("Successful relay grants 60 percent movement"), B->FindComponentByClass<UCombatComponent>()->GetMoveSpeedScale(), 1.6f);
			Test->TestNull(TEXT("Relay claim released"), Cast<AHaulerAIController>(B->GetController())->GetCooperativeTask());
			return true;
		}
		if (!Task.IsValid() && !bCaptured)
		{
			for (TActorIterator<ASharedCarryTask> It(World); It; ++It) { if (It->GetOwner() == Player) { Task = *It; break; } }
		}
		if (!bCaptured && Task.IsValid() && Task->GetPhase() == ESharedCarryPhase::Delivering && Elapsed > 0.75f)
		{
			const UCarrierAnimInstance* Anim = Cast<UCarrierAnimInstance>(A->GetMesh()->GetAnimInstance());
			Test->TestTrue(TEXT("Authored animation is in shared lifting state"), Anim && Anim->bSharedCarry);
			Test->TestEqual(TEXT("One shared cargo, 12 units"), Task->Cargo->GetCurrentItemCount(), 12);
			TSharedPtr<SWindow> Window = GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
			TArray<FColor> Pixels; FIntVector Size(0, 0, 0);
			if (Window && FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(), Pixels, Size))
			{
				TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
				FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir() / TEXT("Expansion_SharedCarry.png")));
			}
			bCaptured = true;
		}
		if (Elapsed < 10.f && Storage->GetStoredItemAmount(EItemType::IronOre) < 12) { return false; }
		Test->TestTrue(TEXT("Paired carry presentation was reached"), bCaptured);
		Test->AddInfo(FString::Printf(TEXT("Pair result: A=%s B=%s task=%s phase=%d cargo=%d"), *A->GetActorLocation().ToString(), *B->GetActorLocation().ToString(),
			Task.IsValid() ? *Task->GetActorLocation().ToString() : TEXT("retired"), Task.IsValid() ? int32(Task->GetPhase()) : -1, Task.IsValid() ? Task->Cargo->GetCurrentItemCount() : 0));
		Test->TestEqual(TEXT("PIE navigated and delivered entire shipment"), Storage->GetStoredItemAmount(EItemType::IronOre), 12);
		Test->TestNull(TEXT("Real AI claim released"), Cast<AHaulerAIController>(A->GetController())->GetCooperativeTask());
		Player->FindComponentByClass<UCooperativeHaulingComponent>()->Configure(nullptr, nullptr);
		Player->Possess(A.Get());
		A->SetActorLocation(FVector(0, 600, 322)); B->SetActorLocation(FVector(100, 600, 322));
		A->GetResourceCarryComponent()->AddItem({EItemType::IronOre, 3});
		URelayHaulingDefinition* Relay = LoadObject<URelayHaulingDefinition>(nullptr, TEXT("/Game/MineLearning/GameplayRuntime/Effects/DA_RelayBaton.DA_RelayBaton"));
		Player->FindComponentByClass<UCooperativeHaulingComponent>()->Configure(Relay, nullptr);
		Started = World->GetTimeSeconds(); Step = 2; return false;
	}
private:
	FAutomationTestBase* Test;
	TWeakObjectPtr<AHaulerCharacter> A, B;
	TWeakObjectPtr<ASharedCarryTask> Task;
	TWeakObjectPtr<UResourceStorageComponent> Storage;
	TWeakObjectPtr<ACameraActor> Camera;
	int32 Step = 0;
	float Started = 0.f;
	bool bCaptured = false;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSharedCarryPIETest, "MineLearning.Expansion.PIEHauling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSharedCarryPIETest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(5.f));
	ADD_LATENT_AUTOMATION_COMMAND(FSharedCarryPIECheck(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
