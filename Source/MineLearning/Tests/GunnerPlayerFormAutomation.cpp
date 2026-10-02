#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "MineLearning/AI/GunnerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SceneComponent.h"
#include "Components/TextBlock.h"
#include "EnhancedInputComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGunnerPlayerFormCombatTest,
	"MineLearning.PlayerForm.GunnerCombatContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGunnerPlayerFormCombatTest::RunTest(const FString& Parameters)
{
	// Exercise the gameplay contract directly; do not synthesize keyboard or mouse input.
	const UWorld::InitializationValues WorldInitialization = UWorld::InitializationValues()
		.AllowAudioPlayback(false)
		.RequiresHitProxies(false)
		.CreatePhysicsScene(true)
		.CreateNavigation(false)
		.CreateAISystem(false)
		.ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		NAME_None,
		nullptr,
		false,
		ERHIFeatureLevel::Num,
		&WorldInitialization);
	if (!TestNotNull(TEXT("Transient combat world"), World))
	{
		return false;
	}

	FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(World);
	const bool bCreatedWorldContext = WorldContext == nullptr;
	if (bCreatedWorldContext)
	{
		WorldContext = &GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext->SetCurrentWorld(World);
	}
	UGameViewportClient* PreviousViewportClient = WorldContext ? WorldContext->GameViewport.Get() : nullptr;
	UGameViewportClient* TestViewportClient = NewObject<UGameViewportClient>(GEngine);
	if (WorldContext)
	{
		WorldContext->GameViewport = TestViewportClient;
	}

	UClass* CrosshairWidgetClass = LoadClass<UUserWidget>(
		nullptr,
		TEXT("/Game/MineLearning/UI/V2/Widgets/WBP_V2_GunnerCrosshair.WBP_V2_GunnerCrosshair_C"));
	if (TestNotNull(TEXT("UMG crosshair class can be loaded"), CrosshairWidgetClass))
	{
		UUserWidget* Crosshair = CreateWidget<UUserWidget>(World, CrosshairWidgetClass);
		if (TestNotNull(TEXT("UMG crosshair can be constructed"), Crosshair))
		{
			UCanvasPanel* CrosshairCanvas = Crosshair->WidgetTree
				? Cast<UCanvasPanel>(Crosshair->WidgetTree->RootWidget)
				: nullptr;
			if (TestNotNull(TEXT("Crosshair visual tree is UMG"), CrosshairCanvas))
			{
				TestEqual(
					TEXT("Crosshair root never intercepts input"),
					CrosshairCanvas->GetVisibility(),
					ESlateVisibility::HitTestInvisible);
			}

			UImage* CrosshairImage = Crosshair->WidgetTree
				? Cast<UImage>(Crosshair->WidgetTree->FindWidget(TEXT("CrosshairImage")))
				: nullptr;
			if (TestNotNull(TEXT("Crosshair is a configurable UMG image"), CrosshairImage))
			{
				UMaterialInterface* CrosshairMaterial = Cast<UMaterialInterface>(
					CrosshairImage->GetBrush().GetResourceObject());
				if (TestNotNull(
					TEXT("Crosshair image has a replaceable brush resource"),
					CrosshairMaterial))
				{
					TestEqual(
						TEXT("Crosshair presentation uses a UI material"),
						CrosshairMaterial->GetMaterial()->MaterialDomain,
						MD_UI);
				}
				TestEqual(
					TEXT("Crosshair image never intercepts input"),
					CrosshairImage->GetVisibility(),
					ESlateVisibility::HitTestInvisible);
			}
		}
	}

	UClass* SkillWidgetClass = LoadClass<UUserWidget>(
		nullptr,
		TEXT("/Game/MineLearning/UI/V2/Widgets/WBP_V2_RobotSkillBar.WBP_V2_RobotSkillBar_C"));
	if (TestNotNull(TEXT("Shared UMG robot skill bar can be loaded"), SkillWidgetClass))
	{
		UUserWidget* SkillBar = CreateWidget<UUserWidget>(World, SkillWidgetClass);
		if (TestNotNull(TEXT("Shared UMG robot skill bar can be constructed"), SkillBar))
		{
			TestNotNull(TEXT("Skill bar uses a UMG Canvas root"), Cast<UCanvasPanel>(SkillBar->WidgetTree->RootWidget));
			TestNotNull(TEXT("Skill bar has a configurable first icon"), SkillBar->WidgetTree->FindWidget(TEXT("Skill1Icon")));
			TestNotNull(TEXT("Skill bar has a configurable second icon"), SkillBar->WidgetTree->FindWidget(TEXT("Skill2Icon")));
			TestNotNull(
				TEXT("Skill one owns a presentation-only quantity badge"),
				Cast<UHorizontalBox>(SkillBar->WidgetTree->FindWidget(TEXT("Skill1QuantityBox"))));
			TestNotNull(
				TEXT("Skill one quantity badge has a configurable icon"),
				Cast<UImage>(SkillBar->WidgetTree->FindWidget(TEXT("Skill1QuantityIcon"))));
			TestNotNull(
				TEXT("Skill one quantity badge has presentation text"),
				Cast<UTextBlock>(SkillBar->WidgetTree->FindWidget(TEXT("Skill1QuantityText"))));
		}
	}

	UClass* AmmoWidgetClass = LoadClass<UUserWidget>(
		nullptr,
		TEXT("/Game/MineLearning/UI/V2/Widgets/WBP_V2_GunnerAmmo.WBP_V2_GunnerAmmo_C"));
	if (TestNotNull(TEXT("Dedicated Gunner ammo UMG can be loaded"), AmmoWidgetClass))
	{
		UUserWidget* AmmoWidget = CreateWidget<UUserWidget>(World, AmmoWidgetClass);
		if (TestNotNull(TEXT("Dedicated Gunner ammo UMG can be constructed"), AmmoWidget))
		{
			TestNotNull(TEXT("Ammo display uses a UMG Border root"), Cast<UBorder>(AmmoWidget->WidgetTree->RootWidget));
			UImage* AmmoIcon = Cast<UImage>(AmmoWidget->WidgetTree->FindWidget(TEXT("AmmoIcon")));
			TestNotNull(TEXT("Ammo display uses a configurable bullet icon"), AmmoIcon ? AmmoIcon->GetBrush().GetResourceObject() : nullptr);
			TestNotNull(TEXT("Ammo display exposes presentation text"), Cast<UTextBlock>(AmmoWidget->WidgetTree->FindWidget(TEXT("AmmoCountText"))));
			const ESlateVisibility AmmoVisibility = AmmoWidget->WidgetTree->RootWidget->GetVisibility();
			TestTrue(TEXT("Ammo root never intercepts input"),
				AmmoVisibility == ESlateVisibility::HitTestInvisible
				|| AmmoVisibility == ESlateVisibility::Hidden
				|| AmmoVisibility == ESlateVisibility::Collapsed);
		}
	}

	UClass* GunnerBlueprintClass = LoadClass<AGunnerCharacter>(
		nullptr,
		TEXT("/Game/MineLearning/Characters/Gunner/Blueprints/BP_Gunner.BP_Gunner_C"));
	if (TestNotNull(TEXT("Gunner Blueprint class can be loaded"), GunnerBlueprintClass))
	{
		const AGunnerCharacter* GunnerBlueprintCDO = GunnerBlueprintClass->GetDefaultObject<AGunnerCharacter>();
		if (TestNotNull(TEXT("Gunner Blueprint CDO"), GunnerBlueprintCDO))
		{
			const USceneComponent* GunnerRootComponent = GunnerBlueprintCDO->GetRootComponent();
			TestNotNull(TEXT("Gunner Blueprint CDO root component"), GunnerRootComponent);
			TestTrue(
				TEXT("Gunner Blueprint defaults to two-times scale"),
				GunnerRootComponent
					&& GunnerRootComponent->GetRelativeScale3D().Equals(FVector(2.0f)));
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGunnerCharacter* Gunner = World->SpawnActor<AGunnerCharacter>(
		AGunnerCharacter::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);

	if (TestNotNull(TEXT("Gunner"), Gunner))
	{
		TestNotNull(
			TEXT("Gameplay exposes ammo changes without knowing an ammo widget"),
			FindFProperty<FMulticastDelegateProperty>(AGunnerCharacter::StaticClass(), TEXT("OnAmmoChanged")));
		TestNotNull(
			TEXT("Gameplay exposes control-mode changes without knowing viewport widgets"),
			FindFProperty<FMulticastDelegateProperty>(AGunnerCharacter::StaticClass(), TEXT("OnControlModeChanged")));
		TestNull(
			TEXT("Gunner gameplay class does not own an ammo widget component"),
			FindFProperty<FObjectProperty>(AGunnerCharacter::StaticClass(), TEXT("AmmoWidgetComponent")));
		TestNull(
			TEXT("Gunner gameplay class does not own a crosshair widget"),
			FindFProperty<FObjectProperty>(AGunnerCharacter::StaticClass(), TEXT("CrosshairWidget")));
		TestNull(
			TEXT("Gunner gameplay class does not own a skill-bar widget"),
			FindFProperty<FObjectProperty>(AGunnerCharacter::StaticClass(), TEXT("PlayerSkillWidget")));



		TestNotNull(TEXT("Player camera boom"), Gunner->FindComponentByClass<USpringArmComponent>());
		TestNotNull(TEXT("Player follow camera"), Gunner->FindComponentByClass<UCameraComponent>());
		UCharacterMovementComponent* Movement = Gunner->GetCharacterMovement();
		if (TestNotNull(TEXT("Character movement"), Movement))
		{
			TestTrue(TEXT("Gunner has a usable walk speed"), Movement->MaxWalkSpeed > 0.0f);
		}

		UEnhancedInputComponent* InputComponent = NewObject<UEnhancedInputComponent>(Gunner);
		Gunner->SetupPlayerInputComponent(InputComponent);
		TestEqual(
			TEXT("Move/look/reload remain Enhanced Input; left mouse fire is removed"),
			InputComponent->GetActionEventBindings().Num(),
			3);

		FInputKeyBinding* FireStartedBinding = nullptr;
		FInputKeyBinding* FireCompletedBinding = nullptr;
		for (FInputKeyBinding& Binding : InputComponent->KeyBindings)
		{
			if (Binding.Chord.Key == EKeys::LeftMouseButton)
			{
				if (Binding.KeyEvent == IE_Pressed) { FireStartedBinding = &Binding; }
				if (Binding.KeyEvent == IE_Released) { FireCompletedBinding = &Binding; }
			}
		}
		for (const auto& Binding : InputComponent->GetActionEventBindings())
		{
			TestFalse(TEXT("Gunner has no old Q action"), Binding->GetAction()->GetName() == TEXT("IA_RobotSkill1"));
		}
		TestNotNull(TEXT("Left mouse pressed"), FireStartedBinding);
		TestNotNull(TEXT("Left mouse released"), FireCompletedBinding);
		APlayerController* PlayerController = World->SpawnActor<APlayerController>(
			APlayerController::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters);
		if (TestNotNull(TEXT("Direct-test player controller"), PlayerController)
			&& FireStartedBinding
			&& FireCompletedBinding)
		{
			ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
			LocalPlayer->PlayerAdded(TestViewportClient, 0);
			PlayerController->SetPlayer(LocalPlayer);
			// The transient test world does not run the normal PIE controller
			// registration path that AddToPlayerScreen uses to resolve its owner.
			World->AddController(PlayerController);
			TestEqual(
				TEXT("Transient local player resolves its controller"),
				LocalPlayer->GetPlayerController(World),
				PlayerController);
			const EMouseCaptureMode CaptureBefore = TestViewportClient->GetMouseCaptureMode();
			const EMouseLockMode LockBefore = TestViewportClient->GetMouseLockMode();
			PlayerController->Possess(Gunner);
			TestFalse(TEXT("Gunner does not override controller cursor policy"), PlayerController->bShowMouseCursor);
			TestTrue(TEXT("Local controller possesses the Gunner form"), PlayerController->GetPawn() == Gunner);
			TestEqual(
				TEXT("Gunner preserves controller mouse capture policy"),
				TestViewportClient->GetMouseCaptureMode(),
				CaptureBefore);
			TestEqual(
				TEXT("Gunner preserves controller mouse lock policy"),
				TestViewportClient->GetMouseLockMode(),
				LockBefore);
			PlayerController->SetControlRotation(FRotator(0.0f, 90.0f, 0.0f));

			Gunner->RestoreLoadedAmmo(20);
			const int32 InitialAmmo = Gunner->GetCurrentAmmo();
			FireStartedBinding->KeyDelegate.Execute(EKeys::LeftMouseButton);
			TestEqual(
				TEXT("Left mouse does not fire before Gunner faces the aim yaw"),
				Gunner->GetCurrentAmmo(),
				InitialAmmo);

			// Exercise a quick tap directly through the bindings. Releasing left mouse must
			// not discard the shot that is still waiting for body alignment.
			FireCompletedBinding->KeyDelegate.Execute(EKeys::LeftMouseButton);
			TestEqual(
				TEXT("Quick left mouse release keeps the pending aligned shot"),
				Gunner->GetCurrentAmmo(),
				InitialAmmo);

			USpringArmComponent* Boom = Gunner->FindComponentByClass<USpringArmComponent>();
			Boom->TickComponent(1.f / 60.f, LEVELTICK_All, nullptr);
			const FTransform CameraBeforeTurn = Gunner->FindComponentByClass<UCameraComponent>()->GetComponentTransform();
			Gunner->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));
			TestTrue(TEXT("Body turn cannot move or rotate the view between spring-arm ticks"),
				CameraBeforeTurn.Equals(Gunner->FindComponentByClass<UCameraComponent>()->GetComponentTransform(), 0.01f));
			Gunner->Tick(1.0f / 60.0f);
			const int32 SpentRounds = InitialAmmo - Gunner->GetCurrentAmmo();
			TestTrue(TEXT("Aligned left mouse attack consumes one or three rounds"), SpentRounds == 1 || SpentRounds == 3);
			if (Movement)
			{
				TestFalse(TEXT("Quick left mouse tap exits controller-facing rotation after firing"), Movement->bUseControllerDesiredRotation);
				TestTrue(TEXT("Quick left mouse tap restores movement-facing rotation after firing"), Movement->bOrientRotationToMovement);
			}

			PlayerController->UnPossess();
			LocalPlayer->PlayerRemoved();
		}

		TestTrue(TEXT("R reload request enters the shared reload state"), Gunner->RequestReload());
		TestTrue(TEXT("Gunner reports reloading"), Gunner->IsReloading());
	}

	if (WorldContext)
	{
		WorldContext->GameViewport = PreviousViewportClient;
	}
	if (bCreatedWorldContext)
	{
		GEngine->DestroyWorldContext(World);
	}
	World->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
