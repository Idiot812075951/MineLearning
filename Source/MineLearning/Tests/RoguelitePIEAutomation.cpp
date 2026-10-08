#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "MineLearning/Combat/WeaponRecoilComponent.h"
#include "MineLearning/Presentation/CombatFeedbackComponent.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "MineLearning/MineLearningPlayerController.h"
#include "MineLearning/Roguelite/MineRunCoordinatorComponent.h"
#include "MineLearning/Roguelite/MetaProgressComponent.h"
#include "MineLearning/Roguelite/RunBuildComponent.h"
#include "MineLearning/Roguelite/UpgradeDraftComponent.h"
#include "MineLearning/Roguelite/RogueliteShop.h"
#include "EngineUtils.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/Demo/DemoRunComponent.h"
#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/AI/PhantomCompanionComponent.h"
#include "MineLearning/AI/UnitRetirementComponent.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/UnitEffectComponent.h"

class FRoguelitePIECheck : public IAutomationLatentCommand
{
public:
	explicit FRoguelitePIECheck(FAutomationTestBase* InTest) : Test(InTest) {}
	~FRoguelitePIECheck() override
	{
		if (MeasuredWeapon.IsValid())
		{
			MeasuredWeapon->OnShotCommitted.Remove(ShotHandle);
		}
	}
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
		AMineLearningPlayerController* Player = World ? Cast<AMineLearningPlayerController>(World->GetFirstPlayerController()) : nullptr;
		if (!Player)
		{
			Test->AddError(TEXT("PIE player missing"));
			return true;
		}
		UMineRunCoordinatorComponent* Coordinator = Player->GetRunCoordinator();
		UDemoRunComponent* Run = Player->GetDemoRun();
		const float Now = World->GetTimeSeconds();
		if (Step == 0)
		{
			URunContentCatalog* Catalog = DuplicateObject(Coordinator->GetCatalog(), GetTransientPackage());
			if (!Catalog)
			{
				Test->AddError(TEXT("Runtime catalog missing"));
				return true;
			}
			Catalog->SaveSlot = TEXT("Automation_PIE_") + FGuid::NewGuid().ToString();
			UMetaProgressComponent* Meta = Coordinator->GetMetaProgress();
			Meta->Initialize(Catalog);
			Meta->AddDebugPoints(10);
			Test->TestTrue(TEXT("Research clone"), Coordinator->Research(TEXT("Clone")));
			Test->TestTrue(TEXT("Research Guren"), Coordinator->Research(TEXT("Guren")));
			Test->TestTrue(TEXT("Research Krypton"), Coordinator->Research(TEXT("Krypton")));
			Test->TestTrue(TEXT("Select another identity"), Coordinator->SelectSummoner(TEXT("Krypton")));
			CheckNameplate(Player->GetPawn(), TEXT("氪金玩家"));
			Test->TestTrue(TEXT("Select free identity"), Coordinator->SelectSummoner(TEXT("Clone")));
			CheckNameplate(Player->GetPawn(), TEXT("复制人"));
			TArray<UUserWidget*> Widgets;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UUserWidget::StaticClass(), true);
			for (UUserWidget* Widget : Widgets)
			{
				if (Widget->GetClass()->GetName().StartsWith(TEXT("WBP_RogueliteHub")))
				{
					Menu = Widget;
					break;
				}
			}
			if (!Test->TestNotNull(TEXT("UMG page in viewport"), Menu.Get()))
			{
				return true;
			}
			UButton* Start = Cast<UButton>(Menu->WidgetTree->FindWidget(TEXT("Start")));
			if (!Test->TestNotNull(TEXT("Start button"), Start))
			{
				return true;
			}
			// Invoke the actual UMG delegate; never move the user's cursor or synthesize OS input.
			Start->OnClicked.Broadcast();
			Test->TestTrue(TEXT("Blueprint button starts run"), Coordinator->GetRunBuild()->IsRunActive());
			Test->TestFalse(TEXT("Identity locked during run"), Coordinator->SelectSummoner(NAME_None));
			Test->TestEqual(TEXT("Identity grants no free upgrade"), Coordinator->GetRunBuild()->GetUpgradeRank(TEXT("Phantom")), 0);
			ARogueliteShop* Shop = nullptr;
			for (TActorIterator<ARogueliteShop> It(World); It; ++It) { Shop = *It; break; }
			if (!Test->TestNotNull(TEXT("Configured cube shop"), Shop)) { return true; }
			Test->TestFalse(TEXT("Remote purchase rejected"), Coordinator->BuyDraft());
			Player->GetPawn()->SetActorLocation(Shop->GetActorLocation() + FVector(180, 0, 100));
			Player->OpenRoguelitePage(ERoguelitePage::Shop);
			UButton* BuySummoner = Cast<UButton>(Menu->WidgetTree->FindWidget(TEXT("BuyDraft")));
			if (!Test->TestNotNull(TEXT("Shared paid shop entry"), BuySummoner))
			{
				return true;
			}
			UResourceStorageComponent* Wallet = Run->GetWarehouse()->GetStorageComponent();
			const int32 EmptyBalance = Wallet->GetAvailableItemAmount(EItemType::Coin);
			BuySummoner->OnClicked.Broadcast();
			Test->TestFalse(TEXT("No money produces no offer"), Coordinator->GetDraft()->HasPendingOffer());
			Test->TestEqual(TEXT("Failed purchase costs nothing"), Wallet->GetAvailableItemAmount(EItemType::Coin), EmptyBalance);
			Coordinator->GrantDebugResources();
			Test->TestTrue(TEXT("Paid Gunner purchase"), Run->ExecuteCommand(EDemoCommand::UnlockGunner));
			Test->TestTrue(TEXT("Paid Guren purchase"), Run->ExecuteCommand(EDemoCommand::UnlockGuren));
			const int32 BeforePurchase = Wallet->GetAvailableItemAmount(EItemType::Coin);
			int32 Paid = 0;
			for (int32 Purchase = 1; Purchase <= 8 && Coordinator->GetRunBuild()->GetUpgradeRank(TEXT("Phantom")) == 0; ++Purchase)
			{
				BuySummoner->OnClicked.Broadcast();
				const TArray<FName> Offer = Coordinator->GetDraft()->GetCandidates();
				Test->TestEqual(TEXT("Three mixed candidates"), Offer.Num(), 3);
				Test->TestTrue(TEXT("Excluded identity never offered"), !Offer.Contains(TEXT("GoldenAK")));
				Paid += Purchase * 4;
				Test->TestEqual(TEXT("Progressive fee"), Wallet->GetAvailableItemAmount(EItemType::Coin), BeforePurchase - Paid);
				Test->TestEqual(TEXT("Automatic choice page"), Player->GetRoguelitePage(), ERoguelitePage::Draft);
				BuySummoner->OnClicked.Broadcast();
				Test->TestEqual(TEXT("No repeated fee"), Wallet->GetAvailableItemAmount(EItemType::Coin), BeforePurchase - Paid);
				Player->CloseRogueliteMenu();
				Player->OpenRoguelitePage(ERoguelitePage::Shop);
				Test->TestEqual(TEXT("Reopening restores choice"), Player->GetRoguelitePage(), ERoguelitePage::Draft);
				// Exhaust non-attack choices, preserving the attack-speed baseline.
				const int32 Index = Offer.Contains(TEXT("Phantom")) ? Offer.IndexOfByKey(FName(TEXT("Phantom")))
					: Offer.IndexOfByPredicate([](FName Id) { return Id != TEXT("AttackHaste50"); });
				UPanelWidget* Candidates = Cast<UPanelWidget>(Menu->WidgetTree->FindWidget(TEXT("Candidates")));
				UUserWidget* Entry = Index >= 0 && Candidates ? Cast<UUserWidget>(Candidates->GetChildAt(Index)) : nullptr;
				UButton* Choose = Entry ? Cast<UButton>(Entry->WidgetTree->FindWidget(TEXT("ChooseButton"))) : nullptr;
				if (!Test->TestNotNull(TEXT("Actual card selection button"), Choose)) { return true; }
				Choose->OnClicked.Broadcast();
			}
			Test->TestEqual(TEXT("Shared pool paid choice grants Phantom"), Coordinator->GetRunBuild()->GetUpgradeRank(TEXT("Phantom")), 1);
			Test->TestTrue(TEXT("Acquired ability excluded from pool"), !Coordinator->GetDraft()->GetEligibilityBlock(TEXT("Phantom")).IsEmpty());
			Player->CloseRogueliteMenu();
			Test->TestTrue(TEXT("Enter empty Gunner without reserves"), Run->ExecuteCommand(EDemoCommand::Gunner));
			CheckWorldAmmo(Player->GetPawn(), 0);
			Test->TestTrue(TEXT("Leave empty Gunner"), Run->ExecuteCommand(EDemoCommand::Human));
			Test->TestTrue(TEXT("Buy initial magazine before transforming"), Run->ExecuteCommand(EDemoCommand::BuyMagazine));
			Test->TestTrue(TEXT("Enter Gunner"), Run->ExecuteCommand(EDemoCommand::Gunner));
			Test->TestNull(TEXT("No human nameplate on Gunner"), FindNameplate(Player->GetPawn()));
			Gunner = Cast<AGunnerCharacter>(Player->GetPawn());
			if (!Test->TestNotNull(TEXT("Player Gunner"), Gunner.Get()))
			{
				return true;
			}
			Test->TestEqual(TEXT("Transformation loads available magazine immediately"), Gunner->GetCurrentAmmo(), 20);
			Test->TestEqual(TEXT("Transformation spends one reserve"), Run->GetReserveMagazines(), 0);
			Test->TestFalse(TEXT("First shot need not wait for reload"), Gunner->IsReloading());
			CheckWorldAmmo(Gunner.Get(), 20);
			const float BeforeMoveScale = Gunner->FindComponentByClass<UCombatComponent>()->GetMoveSpeedScale();
			const float BeforeWalkSpeed = Gunner->GetCharacterMovement()->MaxWalkSpeed;
			Player->BuffAdd(TEXT("MoveHaste50"), -1.f);
			Test->TestTrue(TEXT("Movement card changes actual walking speed"), FMath::IsNearlyEqual(
				Gunner->GetCharacterMovement()->MaxWalkSpeed / BeforeWalkSpeed, (BeforeMoveScale + 0.5f) / BeforeMoveScale));
			Player->BuffRemove(TEXT("MoveHaste50"));
			Test->TestEqual(TEXT("Removing movement buff restores walking speed"), Gunner->GetCharacterMovement()->MaxWalkSpeed, BeforeWalkSpeed);
			MeasuredWeapon = Gunner->FindComponentByClass<UWeaponActionComponent>();
			ShotHandle = MeasuredWeapon->OnShotCommitted.AddLambda([this]()
			{
				ShotTimes.Add(Gunner->GetWorld()->GetTimeSeconds());
			});
			APawn* Phantom = Coordinator->GetPhantoms()->GetCompanion();
			if (!Test->TestNotNull(TEXT("Supported form phantom"), Phantom) || !Gunner.IsValid())
			{
				return true;
			}
			UCombatComponent* Combat = Phantom->FindComponentByClass<UCombatComponent>();
			const UCombatComponent* OwnerCombat = Gunner->FindComponentByClass<UCombatComponent>();
			// Paid drafts may already grant shared speed upgrades before Phantom is offered.
			Test->TestEqual(TEXT("Phantom movement bonus"), Combat->GetMoveSpeedScale(), OwnerCombat->GetMoveSpeedScale() + 0.5f);
			Test->TestEqual(TEXT("Phantom attack bonus"), Combat->GetAttackSpeedScale(), OwnerCombat->GetAttackSpeedScale() + 0.5f);
			Test->TestEqual(TEXT("Phantom spell bonus"), Combat->GetCastSpeedScale(), OwnerCombat->GetCastSpeedScale() + 0.5f);
			Test->TestEqual(TEXT("Baseline is single fire"), Gunner->FindComponentByClass<UWeaponActionComponent>()->GetRoundsPerAttack(), 1);
			Gunner->RestoreLoadedAmmo(20);
			Gunner->SetActorLocation(FVector(800, -1300, 120));
			Gunner->SetActorRotation(FRotator(0, 90, 0));
			Player->SetControlRotation(FRotator(0, 90, 0));
			Until = Now + 1.f;
			Step = 1;
			return false;
		}
		if (Now < Until)
		{
			return false;
		}
		if (Step == 1 || Step == 3)
		{
			ShotTimes.Reset();
			Fire(ETriggerEvent::Started);
			UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/MineLearning/Characters/Gunner/Animations/AM_Gunner_Fire.AM_Gunner_Fire"));
			const float Rate = Gunner->GetMesh()->GetAnimInstance()->Montage_GetPlayRate(Montage);
			if (Step == 1)
			{
				BaseAnimationRate = Rate;
				Test->TestEqual(TEXT("First left mouse immediately fires a round"), Gunner->GetCurrentAmmo(), 19);
			}
			else
			{
				Test->TestTrue(TEXT("Fire animation also speeds up by 50 percent"), FMath::IsNearlyEqual(Rate / BaseAnimationRate, 1.5f, 0.01f));
			}
			Until = Now + 4.f;
			++Step;
			return false;
		}
		if (Step == 2)
		{
			Fire(ETriggerEvent::Completed);
			BaselineShots = 20 - Gunner->GetCurrentAmmo();
			BaseShotInterval = MeanShotInterval();
			Test->TestTrue(TEXT("Authored baseline fires every 0.7 seconds"), FMath::IsNearlyEqual(BaseShotInterval, 0.7f, 0.05f));
			Test->TestTrue(TEXT("Held left mouse emits repeatedly"), BaselineShots > 1);
			Player->BuffAdd(TEXT("AttackHaste50"), -1.f);
			Test->TestEqual(TEXT("Attack buff effective"), Gunner->FindComponentByClass<UCombatComponent>()->GetAttackSpeedScale(), 1.5f);
			UTextBlock* BuffText = Cast<UTextBlock>(Menu->WidgetTree->FindWidget(TEXT("BuffText")));
			Test->TestTrue(TEXT("Buff event updates UMG"), BuffText && !BuffText->GetText().IsEmpty());
			Gunner->RestoreLoadedAmmo(20);
			Until = Now + 1.f;
			Step = 3;
			return false;
		}
		if (Step == 4)
		{
			Fire(ETriggerEvent::Completed);
			const int32 FastShots = 20 - Gunner->GetCurrentAmmo();
			Test->AddInfo(FString::Printf(TEXT("Held left mouse rounds in 4 seconds: baseline %d, +50%% attack %d"), BaselineShots, FastShots));
			Test->TestTrue(TEXT("Attack speed increases held-left-mouse cadence"), FastShots > BaselineShots);
			const float FastInterval = MeanShotInterval();
			Test->AddInfo(FString::Printf(TEXT("Measured cadence: %.3fs -> %.3fs, rate multiplier %.3f; animation multiplier 1.5"),
				BaseShotInterval, FastInterval, BaseShotInterval / FastInterval));
			Test->TestTrue(TEXT("Measured attack rate is 1.5x within frame tolerance"), FMath::IsNearlyEqual(BaseShotInterval / FastInterval, 1.5f, 0.1f));
			Until = Now + 1.f;
			Step = 40;
			return false;
		}
		if (Step == 40)
		{
			Player->BuffAdd(TEXT("TripleShot"), -1.f);
			Test->TestTrue(TEXT("Unlocked pattern defaults to burst"), Gunner->IsBurstMode());
			CheckFireModeUI(World, TEXT("三连发"));
			Gunner->RestoreLoadedAmmo(20);
			ShotTimes.Reset();
			Fire(ETriggerEvent::Started);
			Until = Now + 2.f;
			Step = 41;
			return false;
		}
		if (Step == 41)
		{
			CaptureNameplate(TEXT("Roguelite_BurstMode.png"));
			Test->TestEqual(TEXT("Held burst left mouse emits exactly one three-round volley"), 20 - Gunner->GetCurrentAmmo(), 3);
			Test->TestTrue(TEXT("Burst bullets use twice single-fire attack rate with cap"),
				FMath::IsNearlyEqual(MeanShotInterval(), FMath::Max(0.2f, 0.7f / 1.5f / 2.f), 0.035f));
			Fire(ETriggerEvent::Completed);
			bool bFoundToggle = false;
			for (FInputKeyBinding& Binding : Player->InputComponent->KeyBindings)
			{
				if (Binding.Chord.Key == EKeys::RightMouseButton && Binding.KeyEvent == IE_Released)
				{
					Binding.KeyDelegate.Execute(EKeys::RightMouseButton);
					bFoundToggle = true;
					break;
				}
			}
			Test->TestTrue(TEXT("Real right-mouse binding switches to single"), bFoundToggle && !Gunner->IsBurstMode());
			CheckFireModeUI(World, TEXT("单发"));
			Test->TestFalse(TEXT("Selected mode belongs to exported weapon state"), MeasuredWeapon->Export().bBurstEnabled);
			Gunner->RestoreLoadedAmmo(20);
			Fire(ETriggerEvent::Started);
			Until = Now + 2.f;
			Step = 42;
			return false;
		}
		if (Step == 42)
		{
			Fire(ETriggerEvent::Completed);
			Test->TestTrue(TEXT("Single mode still repeats while left mouse held"), 20 - Gunner->GetCurrentAmmo() > 3);
			MeasuredWeapon->OnShotCommitted.Remove(ShotHandle);
			Player->BuffRemove(TEXT("TripleShot"));
			Test->TestFalse(TEXT("Revoking upgrade removes switching permission"), MeasuredWeapon->ToggleBurstMode());
			Player->BuffAdd(TEXT("Roamer"), -1.f);
			Player->BuffAdd(TEXT("Conqueror"), -1.f);
			UCombatComponent* Combat = Gunner->FindComponentByClass<UCombatComponent>();
			Gunner->GetCharacterMovement()->Velocity = Gunner->GetActorRightVector() * 100.f;
			for (int32 I = 0; I < 10; ++I) { Combat->NotifyAttackResolved(true); }
			Gunner->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			Combat->NotifyAttackOutOfRange();
			Gunner->FindComponentByClass<UWeaponRecoilComponent>()->ApplyShot(FVector::ForwardVector);
			Until = Now + 0.2f;
			Step = 43;
			return false;
		}
		if (Step == 43)
		{
			CaptureNameplate(TEXT("Roguelite_CombatFeedback.png"));
			UPanelWidget* BuffList = Cast<UPanelWidget>(Menu->WidgetTree->FindWidget(TEXT("BuffList")));
			UUnitEffectComponent* Effects = Gunner->FindComponentByClass<UUnitEffectComponent>();
			Test->TestTrue(TEXT("HUD renders one row per active buff"), BuffList && BuffList->GetChildrenCount() == Effects->GetActiveEffectViews().Num());
			bool bRoamerShown = false;
			if (BuffList)
			{
				for (UWidget* Child : BuffList->GetAllChildren())
				{
					UUserWidget* Row = Cast<UUserWidget>(Child);
					UImage* Icon = Row ? Cast<UImage>(Row->WidgetTree->FindWidget(TEXT("BuffIcon"))) : nullptr;
					UTextBlock* Text = Row ? Cast<UTextBlock>(Row->WidgetTree->FindWidget(TEXT("Status"))) : nullptr;
					Test->TestTrue(TEXT("Buff row has actual icon and text"), Icon && Icon->GetBrush().GetResourceObject() && Text && !Text->GetText().IsEmpty());
					bRoamerShown |= Text && Text->GetText().ToString().Contains(TEXT("漫游枪手"));
				}
			}
			Test->TestTrue(TEXT("Roamer status visible"), bRoamerShown);
			TArray<UUserWidget*> Widgets;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UUserWidget::StaticClass(), false);
			bool bExpanded = false;
			bool bLeftMouseLabel = false;
			for (UUserWidget* Widget : Widgets)
			{
				if (!Widget->WidgetTree || Widget->GetOwningPlayer() != Player) { continue; }
				if (UImage* Image = Cast<UImage>(Widget->WidgetTree->FindWidget(TEXT("CrosshairImage"))))
				{
					bExpanded |= Image->GetRenderTransform().Scale.X > 1.f;
				}
				if (UTextBlock* Key = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("Skill1Key"))))
				{
					bLeftMouseLabel |= Key->GetText().ToString() == TEXT("左键");
				}
			}
			Test->TestTrue(TEXT("Actual UMG crosshair expands on recoil event"), bExpanded);
			Test->TestTrue(TEXT("Actual skill bar says left mouse"), bLeftMouseLabel);
			UCombatFeedbackComponent* Feedback = Gunner->FindComponentByClass<UCombatFeedbackComponent>();
			Test->TestTrue(TEXT("Assembled presentation shows ring and aura"), Feedback && Feedback->IsRangeVisible() && Feedback->GetAuraIntensity() > 0.f);
			Gunner->FindComponentByClass<UCombatComponent>()->NotifyAttackResolved(false, false);
			Test->TestEqual(TEXT("Miss also clears assembled aura"), Feedback->GetAuraIntensity(), 0.f);
			Player->BuffRemove(TEXT("Roamer"));
			Player->BuffRemove(TEXT("Conqueror"));
			Until = Now + 2.1f;
			Step = 44;
			return false;
		}
		if (Step == 44)
		{
			Test->TestFalse(TEXT("Live HUD scenario range ring expires"), Gunner->FindComponentByClass<UCombatFeedbackComponent>()->IsRangeVisible());
			Step = 5;
			return false;
		}
		if (Step == 5)
		{
			Test->TestTrue(TEXT("Return to human"), Run->ExecuteCommand(EDemoCommand::Human));
			CheckNameplate(Player->GetPawn(), TEXT("复制人"));
			Test->TestNull(TEXT("No phantom on human"), Coordinator->GetPhantoms()->GetCompanion());
			Until = Now + 1.f;
			Step = 6;
			return false;
		}
		if (Step == 6)
		{
			CaptureNameplate();
			Test->TestTrue(TEXT("Enter unsupported AI Guren"), Run->ExecuteCommand(EDemoCommand::Guren));
			Test->TestNull(TEXT("No Guren phantom"), Coordinator->GetPhantoms()->GetCompanion());
			Test->TestTrue(TEXT("Return to Gunner"), Run->ExecuteCommand(EDemoCommand::Gunner));
			Retiring = Coordinator->GetPhantoms()->GetCompanion();
			if (Retiring.IsValid())
			{
				Coordinator->GetPhantoms()->SpawnFor(Player->GetPawn(), 0.05f);
			}
			Until = Now + 1.f;
			Step = 7;
			return false;
		}
		Test->TestNull(TEXT("Lifetime clears companion"), Coordinator->GetPhantoms()->GetCompanion());
		Test->TestTrue(TEXT("Replaced phantom is destroyed"), !Retiring.IsValid());
		Test->TestTrue(TEXT("Clean isolated PIE save"), Coordinator->GetMetaProgress()->ClearProfile());
		return true;
	}
private:
	void CheckFireModeUI(UWorld* World, const TCHAR* Expected)
	{
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UUserWidget::StaticClass(), false);
		bool bFound = false;
		for (UUserWidget* Widget : Widgets)
		{
			if (!Widget->WidgetTree || Widget->GetOwningPlayer() != World->GetFirstPlayerController()) { continue; }
			UTextBlock* Label = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("FireMode")));
			if (Label && Label->GetText().ToString().Contains(Expected) && Label->GetVisibility() == ESlateVisibility::HitTestInvisible) { bFound = true; }
		}
		Test->TestTrue(TEXT("UMG reacts to fire mode state event"), bFound);
	}
	void CheckWorldAmmo(APawn* Pawn, int32 Expected)
	{
		TInlineComponentArray<UWidgetComponent*> Components(Pawn);
		for (UWidgetComponent* Component : Components)
		{
			UUserWidget* Widget = Component->GetUserWidgetObject();
			UTextBlock* Count = Widget ? Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("AmmoCountText"))) : nullptr;
			if (Count)
			{
				Test->AddInfo(FString::Printf(TEXT("World ammo UI: %s (expected %d)"), *Count->GetText().ToString(), Expected));
				Test->TestEqual(TEXT("World ammo widget reflects restored/loaded state"), Count->GetText().ToString(), FString::Printf(TEXT("×%d"), Expected));
				return;
			}
		}
		Test->AddError(TEXT("World ammo count widget missing"));
	}
	float MeanShotInterval() const
	{
		return ShotTimes.Num() > 1 ? (ShotTimes.Last() - ShotTimes[0]) / (ShotTimes.Num() - 1) : 0.f;
	}
	void CaptureNameplate(const TCHAR* Filename = TEXT("SummonerNameplate.png")) const
	{
		// Target the PIE window explicitly; global screenshot requests can be consumed by an editor viewport.
		TSharedPtr<SWindow> Window = GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
		TArray<FColor> Pixels;
		FIntVector Size(0, 0, 0);
		if (Window && FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(), Pixels, Size))
		{
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
			FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir() / Filename));
		}
	}

	UWidgetComponent* FindNameplate(APawn* Pawn) const
	{
		if (Pawn)
		{
			TInlineComponentArray<UWidgetComponent*> Components(Pawn);
			for (UWidgetComponent* Component : Components)
			{
				if (Component->GetFName() == TEXT("SummonerNameplate"))
				{
					return Component;
				}
			}
		}
		return nullptr;
	}

	void CheckNameplate(APawn* Pawn, const TCHAR* Expected)
	{
		UWidgetComponent* Component = FindNameplate(Pawn);
		UUserWidget* Widget = Component ? Component->GetUserWidgetObject() : nullptr;
		UTextBlock* Label = Widget ? Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("SummonerName"))) : nullptr;
		Test->TestTrue(TEXT("Human nameplate reflects selected identity"), Label && Label->GetText().ToString() == Expected);
	}

	void Fire(ETriggerEvent Trigger)
	{
		const EInputEvent KeyEvent = Trigger == ETriggerEvent::Started ? IE_Pressed : IE_Released;
		for (FInputKeyBinding& Binding : Gunner->InputComponent->KeyBindings)
		{
			if (Binding.Chord.Key == EKeys::LeftMouseButton && Binding.KeyEvent == KeyEvent)
			{
				Binding.KeyDelegate.Execute(EKeys::LeftMouseButton);
				return;
			}
		}
		Test->AddError(TEXT("Gunner left mouse binding missing"));
	}
	FAutomationTestBase* Test;
	TWeakObjectPtr<UUserWidget> Menu;
	TWeakObjectPtr<AGunnerCharacter> Gunner;
	TWeakObjectPtr<UWeaponActionComponent> MeasuredWeapon;
	FDelegateHandle ShotHandle;
	TArray<float> ShotTimes;
	float BaseShotInterval = 0.f;
	float BaseAnimationRate = 1.f;
	TWeakObjectPtr<APawn> Retiring;
	int32 Step = 0;
	int32 BaselineShots = 0;
	float Until = 0.f;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoguelitePIETest, "MineLearning.Roguelite.PIEIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRoguelitePIETest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FRoguelitePIECheck(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
