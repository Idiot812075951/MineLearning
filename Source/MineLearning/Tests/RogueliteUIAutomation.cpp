#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/InputComponent.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "MineLearning/MineLearningPlayerController.h"
#include "MineLearning/Roguelite/MineRunCoordinatorComponent.h"
#include "MineLearning/Roguelite/MetaProgressComponent.h"
#include "MineLearning/Roguelite/RunBuildComponent.h"
#include "MineLearning/Roguelite/UpgradeDraftComponent.h"
#include "MineLearning/Roguelite/RogueliteShop.h"
#include "MineLearning/Demo/DemoRunComponent.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/AI/HaulerCharacter.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Mining/MiningToolComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"

// Exercise the shipped widget delegates and input bindings without simulated desktop input.
class FRogueliteUIFlow : public IAutomationLatentCommand
{
public:
	explicit FRogueliteUIFlow(FAutomationTestBase* InTest) : Test(InTest) {}

	bool Update() override
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE)
			{
				World = Context.World();
				break;
			}
		}
		Player = World ? Cast<AMineLearningPlayerController>(World->GetFirstPlayerController()) : nullptr;
		if (!Player || World->GetTimeSeconds() < Until)
		{
			return !Player;
		}
		UMineRunCoordinatorComponent* Coordinator = Player->GetRunCoordinator();
		if (Step == 0)
		{
			Test->TestTrue(TEXT("Opening preparation shows cursor"), Player->bShowMouseCursor);
			TArray<UUserWidget*> Widgets;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UUserWidget::StaticClass(), true);
			for (UUserWidget* Widget : Widgets)
			{
				if (Widget->GetClass()->GetName().StartsWith(TEXT("WBP_RogueliteHub")))
				{
					Menu = Widget;
				}
			}
			if (!Test->TestNotNull(TEXT("Shipped UMG hub"), Menu.Get()))
			{
				return true;
			}
			URunContentCatalog* Catalog = DuplicateObject(Coordinator->GetCatalog(), GetTransientPackage());
			Catalog->SaveSlot = TEXT("Automation_UI_") + FGuid::NewGuid().ToString();
			Coordinator->GetMetaProgress()->Initialize(Catalog);
			Coordinator->GetMetaProgress()->AddDebugPoints(20);
			Player->OpenRoguelitePage(ERoguelitePage::Preparation);
		}
		else if (Step == 1)
		{
			Capture(TEXT("Roguelite_Preparation.png"));
			Click(Menu.Get(), TEXT("NavTalents"));
			Test->TestEqual(TEXT("Talent navigation"), Player->GetRoguelitePage(), ERoguelitePage::Talents);
			UPanelWidget* Nodes = Cast<UPanelWidget>(Find(Menu.Get(), TEXT("Talents")));
			Test->TestEqual(TEXT("Nodes plus prerequisite links"), Nodes ? Nodes->GetChildrenCount() : 0,
				Coordinator->GetTalentNodes().Num() + Coordinator->GetTalentLinks().Num());
			if (Nodes)
			{
				for (UWidget* Child : Nodes->GetAllChildren())
				{
					UUserWidget* Node = Cast<UUserWidget>(Child);
					FNameProperty* IdProperty = Node ? FindFProperty<FNameProperty>(Node->GetClass(), TEXT("Id")) : nullptr;
					if (IdProperty && IdProperty->GetPropertyValue_InContainer(Node) == TEXT("Clone"))
					{
						Click(Node, TEXT("ChooseButton"));
						break;
					}
				}
			}
			Test->TestTrue(TEXT("UMG talent unlock persists"), Coordinator->GetMetaProgress()->IsResearched(TEXT("Clone")));
		}
		else if (Step == 2)
		{
			Capture(TEXT("Roguelite_Talents.png"));
			Player->OpenRoguelitePage(ERoguelitePage::Preparation);
			UPanelWidget* Identities = Cast<UPanelWidget>(Find(Menu.Get(), TEXT("Summoners")));
			UUserWidget* Clone = nullptr;
			if (Identities)
			{
				for (UWidget* Child : Identities->GetAllChildren())
				{
					UUserWidget* Card = Cast<UUserWidget>(Child);
					FNameProperty* IdProperty = Card ? FindFProperty<FNameProperty>(Card->GetClass(), TEXT("Id")) : nullptr;
					if (IdProperty && IdProperty->GetPropertyValue_InContainer(Card) == TEXT("Clone"))
					{
						Clone = Card;
						break;
					}
				}
			}
			Click(Clone, TEXT("ChooseButton"));
			Test->TestEqual(TEXT("Free clone identity selection"), Coordinator->GetMetaProgress()->GetSelectedSummoner(), FName(TEXT("Clone")));
			Click(Menu.Get(), TEXT("Start"));
			Test->TestTrue(TEXT("Run starts via UMG"), Coordinator->GetRunBuild()->IsRunActive());
			Test->TestFalse(TEXT("Start closes UI and hides cursor"), Player->bShowMouseCursor);
			Test->TestFalse(TEXT("Gameplay restores look input"), Player->IsLookInputIgnored());
			Test->TestFalse(TEXT("Talent changes frozen"), Coordinator->Research(TEXT("Collector")));
			ARogueliteShop* Shop = nullptr;
			for (TActorIterator<ARogueliteShop> It(World); It; ++It)
			{
				Shop = *It;
				break;
			}
			if (!Test->TestNotNull(TEXT("Configured world cube"), Shop))
			{
				Coordinator->GetMetaProgress()->ClearProfile();
				return true;
			}
			Player->GetPawn()->SetActorLocation(Shop->GetActorLocation() + FVector(180, 0, 100));
			Press(EKeys::E);
			Test->TestEqual(TEXT("E opens cube shop"), Player->GetRoguelitePage(), ERoguelitePage::Shop);
			Test->TestTrue(TEXT("Shop exposes cursor and blocks look"), Player->bShowMouseCursor && Player->IsLookInputIgnored());
			UButton* Buy = Cast<UButton>(Find(Menu.Get(), TEXT("BuyDraft")));
			Test->TestTrue(TEXT("Cannot afford button disabled"), Buy && !Buy->GetIsEnabled());
		}
		else if (Step == 3)
		{
			Capture(TEXT("Roguelite_Shop.png"));
			Coordinator->GrantDebugResources();
			Click(Menu.Get(), TEXT("BuyDraft"));
			Test->TestEqual(TEXT("Paid offer opens three cards"), Player->GetRoguelitePage(), ERoguelitePage::Draft);
			UPanelWidget* Cards = Cast<UPanelWidget>(Find(Menu.Get(), TEXT("Candidates")));
			Test->TestEqual(TEXT("Three visual cards"), Cards ? Cards->GetChildrenCount() : 0, 3);
			if (Cards)
			{
				for (UWidget* Child : Cards->GetAllChildren())
				{
					UUserWidget* Card = Cast<UUserWidget>(Child);
					UImage* Icon = Cast<UImage>(Find(Card, TEXT("Icon")));
					Test->TestTrue(TEXT("Card uses configured icon material"), Icon && Icon->GetBrush().GetResourceObject());
				}
			}
		}
		else if (Step == 4)
		{
			Capture(TEXT("Roguelite_Draft.png"));
			UPanelWidget* Cards = Cast<UPanelWidget>(Find(Menu.Get(), TEXT("Candidates")));
			Click(Cards ? Cast<UUserWidget>(Cards->GetChildAt(0)) : nullptr, TEXT("ChooseButton"));
			Test->TestFalse(TEXT("Choice consumes offer"), Coordinator->GetDraft()->HasPendingOffer());
			const TArray<FItemStack> NextCost = Coordinator->GetDraft()->GetNextCost();
			if (Test->TestEqual(TEXT("Configured price available"), NextCost.Num(), 1))
			{
				Test->TestEqual(TEXT("Next purchase is eight"), NextCost[0].Amount, 8);
			}
			Click(Menu.Get(), TEXT("NavCodex"));
			Test->TestEqual(TEXT("Codex navigation"), Player->GetRoguelitePage(), ERoguelitePage::Codex);
			UPanelWidget* Available = Cast<UPanelWidget>(Find(Menu.Get(), TEXT("AvailableCards")));
			UPanelWidget* Locked = Cast<UPanelWidget>(Find(Menu.Get(), TEXT("LockedCards")));
			Test->TestEqual(TEXT("Codex includes every configured upgrade"),
				(Available ? Available->GetChildrenCount() : 0) + (Locked ? Locked->GetChildrenCount() : 0), Coordinator->GetCatalog()->GetUpgradeIds().Num());
			Test->TestTrue(TEXT("Other identity shows a reason"), !Coordinator->GetDraft()->GetEligibilityBlock(TEXT("GoldenAK")).IsEmpty());
			for (FName Id : {FName(TEXT("GoldenAK")), FName(TEXT("TripleShot")), FName(TEXT("SuperRound")), FName(TEXT("SuperMagazine"))})
			{
				UUserWidget* Card = FindEntry(Locked, Id);
				UTextBlock* Badge = Card ? Cast<UTextBlock>(Find(Card, TEXT("UnitBadge"))) : nullptr;
				Test->TestTrue(TEXT("Weapon cards visibly identify Gunner"), Badge && Badge->GetText().ToString() == TEXT("Gunner"));
			}
		}
		else if (Step == 5)
		{
			Capture(TEXT("Roguelite_Codex.png"));
			if (UScrollBox* Scroll = Cast<UScrollBox>(Find(Menu.Get(), TEXT("CodexScroll"))))
			{
				Scroll->ScrollToEnd();
			}
		}
		else if (Step == 6)
		{
			Capture(TEXT("Roguelite_CodexLocked.png"));
			Player->CloseRogueliteMenu();
			Test->TestFalse(TEXT("Closing codex hides cursor"), Player->bShowMouseCursor);
			Press(EKeys::Tab);
			Test->TestTrue(TEXT("Terminal exposes cursor"), Player->bShowMouseCursor);
		}
		else if (Step == 7)
		{
			Capture(TEXT("Roguelite_Terminal.png"));
			Press(EKeys::Tab);
			Player->OpenRoguelitePage(ERoguelitePage::Debug);
			Test->TestTrue(TEXT("Return to human for input checks"), Player->GetDemoRun()->ExecuteCommand(EDemoCommand::Human));
			Player->CloseRogueliteMenu();
			Player->ToggleCombatDetails();
			Test->TestTrue(TEXT("Inspection exposes cursor"), Player->bShowMouseCursor);
			Player->OpenRoguelitePage(ERoguelitePage::Codex);
			Player->CloseRogueliteMenu();
			Test->TestTrue(TEXT("Cursor remains while another panel is open"), Player->bShowMouseCursor);
			Press(EKeys::Escape);
			Test->TestFalse(TEXT("Escape closes final panel and hides cursor"), Player->bShowMouseCursor);
			OriginalPawn = Player->GetPawn();
			UClass* BuddyClass = LoadClass<AMiningCompanionCharacter>(nullptr, TEXT("/Game/MineLearning/Characters/OreBuddy/Blueprints/BP_OreBuddy07.BP_OreBuddy07_C"));
			Player->Possess(World->SpawnActor<AMiningCompanionCharacter>(BuddyClass, FVector(0, 0, 1500), FRotator::ZeroRotator));
		}
		else if (Step == 8)
		{

			CheckPrimaryInput();
			Test->TestFalse(TEXT("OreBuddy possession preserves gameplay cursor"), Player->bShowMouseCursor);
			APawn* Buddy = Player->GetPawn();
			Player->OpenRoguelitePage(ERoguelitePage::Codex);
			Player->Possess(World->SpawnActor<AHaulerCharacter>(FVector(0, 0, 1500), FRotator::ZeroRotator));
			Buddy->Destroy();
		}
		else if (Step == 9)
		{
			Test->TestTrue(TEXT("Possession with menu open preserves visible cursor"), Player->bShowMouseCursor);
			Test->TestFalse(TEXT("Menu blocks new pawn input"), Player->GetPawn()->InputEnabled());
			Player->CloseRogueliteMenu();
			CheckPrimaryInput();
			APawn* Carrier = Player->GetPawn();
			Player->Possess(OriginalPawn.Get());
			Carrier->Destroy();
		}
		else if (Step == 10)
		{
			Test->TestFalse(TEXT("Returning to human preserves hidden cursor"), Player->bShowMouseCursor);
			Test->TestFalse(TEXT("Returning to gameplay restores look"), Player->IsLookInputIgnored());
			Player->OpenRoguelitePage(ERoguelitePage::Debug);
			Click(Menu.Get(), TEXT("ClearSave"));
			Click(Menu.Get(), TEXT("ConfirmClear"));
			Test->TestFalse(TEXT("UMG clears isolated talent profile"), Coordinator->GetMetaProgress()->IsResearched(TEXT("Clone")));
			Test->TestFalse(TEXT("Clear ends run"), Coordinator->GetRunBuild()->IsRunActive());
			return true;
		}
		++Step;
		Until = World->GetTimeSeconds() + 1.f;
		return false;
	}

private:
	void CheckPrimaryInput()
	{
		APawn* Pawn = Player->GetPawn();
		UInputComponent* Input = Pawn ? Pawn->InputComponent : nullptr;
		if (!Test->TestNotNull(TEXT("Possessed unit input"), Input)) { return; }
		bool bFoundPrimary = false;
		bool bResolvedAttack = false;
		UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>();
		const FDelegateHandle Handle = Combat->OnPrimaryAttackResolved.AddLambda([&bResolvedAttack](bool, bool) { bResolvedAttack = true; });
		for (FInputKeyBinding& Binding : Input->KeyBindings)
		{
			Test->TestFalse(TEXT("Unit has no direct Q binding"), Binding.Chord.Key == EKeys::Q);
			if (Binding.Chord.Key == EKeys::LeftMouseButton && Binding.KeyEvent == IE_Pressed)
			{
				bFoundPrimary = true;
				Binding.KeyDelegate.Execute(EKeys::LeftMouseButton);
			}
		}
		Combat->OnPrimaryAttackResolved.Remove(Handle);
		const UMiningToolComponent* Mining = Pawn->FindComponentByClass<UMiningToolComponent>();
		Test->TestTrue(TEXT("Left mouse executes the primary action against empty space"), bFoundPrimary && (Mining ? Mining->IsMining() : bResolvedAttack));
		if (UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(Input))
		{
			for (const auto& Binding : Enhanced->GetActionEventBindings())
			{
				Test->TestFalse(TEXT("Old enhanced Q binding removed"), Binding->GetAction()->GetName() == TEXT("IA_RobotSkill1"));
			}
		}
	}
	UUserWidget* FindEntry(UPanelWidget* Container, FName Id) const
	{
		if (Container)
		{
			for (UWidget* Child : Container->GetAllChildren())
			{
				UUserWidget* Card = Cast<UUserWidget>(Child);
				FNameProperty* Property = Card ? FindFProperty<FNameProperty>(Card->GetClass(), TEXT("Id")) : nullptr;
				if (Property && Property->GetPropertyValue_InContainer(Card) == Id)
				{
					return Card;
				}
			}
		}
		return nullptr;
	}
	UWidget* Find(UUserWidget* Owner, FName Name) const
	{
		return Owner && Owner->WidgetTree ? Owner->WidgetTree->FindWidget(Name) : nullptr;
	}
	void Click(UUserWidget* Owner, FName Name)
	{
		UButton* Button = Cast<UButton>(Find(Owner, Name));
		if (Test->TestNotNull(*FString::Printf(TEXT("UMG button %s"), *Name.ToString()), Button))
		{
			Button->OnClicked.Broadcast();
		}
	}
	void Press(FKey Key)
	{
		for (FInputKeyBinding& Binding : Player->InputComponent->KeyBindings)
		{
			if (Binding.Chord.Key == Key && Binding.KeyEvent == IE_Pressed)
			{
				Binding.KeyDelegate.Execute(Key);
				return;
			}
		}
		Test->AddError(FString::Printf(TEXT("Native key binding missing: %s"), *Key.ToString()));
	}
	void Capture(const TCHAR* Name)
	{
		TSharedPtr<SWindow> Window = GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
		TArray<FColor> Pixels;
		FIntVector Size(0, 0, 0);
		if (Window && FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(), Pixels, Size))
		{
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
			Test->TestTrue(TEXT("PIE screen capture"), FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir() / Name)));
		}
		else
		{
			Test->AddError(TEXT("PIE window capture failed"));
		}
	}
	FAutomationTestBase* Test;
	AMineLearningPlayerController* Player = nullptr;
	TWeakObjectPtr<UUserWidget> Menu;
	TWeakObjectPtr<APawn> OriginalPawn;
	int32 Step = 0;
	float Until = 0.f;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRogueliteUIFlowTest, "MineLearning.Roguelite.UIFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRogueliteUIFlowTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FRogueliteUIFlow(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
