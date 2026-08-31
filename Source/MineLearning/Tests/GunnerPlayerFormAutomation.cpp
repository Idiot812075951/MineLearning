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
		TEXT("/Game/MineLearning/UI/Gunner/WBP_GunnerCrosshair.WBP_GunnerCrosshair_C"));
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
		TEXT("/Game/MineLearning/UI/RobotSkills/WBP_RobotSkillBar.WBP_RobotSkillBar_C"));
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
		TEXT("/Game/MineLearning/UI/Gunner/WBP_GunnerAmmo.WBP_GunnerAmmo_C"));
	if (TestNotNull(TEXT("Dedicated Gunner ammo UMG can be loaded"), AmmoWidgetClass))
	{
		UUserWidget* AmmoWidget = CreateWidget<UUserWidget>(World, AmmoWidgetClass);
		if (TestNotNull(TEXT("Dedicated Gunner ammo UMG can be constructed"), AmmoWidget))
		{
			TestNotNull(TEXT("Ammo display uses a UMG Border root"), Cast<UBorder>(AmmoWidget->WidgetTree->RootWidget));
			UImage* AmmoIcon = Cast<UImage>(AmmoWidget->WidgetTree->FindWidget(TEXT("AmmoIcon")));
			TestNotNull(TEXT("Ammo display uses a configurable bullet icon"), AmmoIcon ? AmmoIcon->GetBrush().GetResourceObject() : nullptr);
			TestNotNull(TEXT("Ammo display exposes presentation text"), Cast<UTextBlock>(AmmoWidget->WidgetTree->FindWidget(TEXT("AmmoCountText"))));
			TestEqual(TEXT("Ammo root never intercepts input"), AmmoWidget->WidgetTree->RootWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);
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

		const FFloatProperty* BurstChanceProperty = FindFProperty<FFloatProperty>(
			AGunnerCharacter::StaticClass(),
			TEXT("BurstChance"));
		if (TestNotNull(TEXT("Burst probability remains configurable"), BurstChanceProperty))
		{
			TestTrue(
				TEXT("Default attack has the established 50 percent burst chance"),
				FMath::IsNearlyEqual(BurstChanceProperty->GetPropertyValue_InContainer(Gunner), 0.5f));
		}

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
			TEXT("Five player actions provide seven trigger bindings"),
			InputComponent->GetActionEventBindings().Num(),
			7);

		const FEnhancedInputActionEventBinding* FireStartedBinding = nullptr;
		const FEnhancedInputActionEventBinding* FireCompletedBinding = nullptr;
		for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : InputComponent->GetActionEventBindings())
		{
			if (!Binding || !Binding->GetAction()
				|| Binding->GetAction()->GetName() != TEXT("IA_RobotSkill1"))
			{
				continue;
			}

			if (Binding->GetTriggerEvent() == ETriggerEvent::Started)
			{
				FireStartedBinding = Binding.Get();
			}
			else if (Binding->GetTriggerEvent() == ETriggerEvent::Completed)
			{
				FireCompletedBinding = Binding.Get();
			}
		}

		TestNotNull(TEXT("Q fire Started binding"), FireStartedBinding);
		TestNotNull(TEXT("Q fire Completed binding"), FireCompletedBinding);
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
			PlayerController->Possess(Gunner);
			TestTrue(TEXT("Gunner keeps the same visible cursor as the human form"), PlayerController->bShowMouseCursor);
			TestTrue(TEXT("Local controller possesses the Gunner form"), PlayerController->GetPawn() == Gunner);
			TestEqual(
				TEXT("Mouse capture begins only while a mouse button is held"),
				TestViewportClient->GetMouseCaptureMode(),
				EMouseCaptureMode::CaptureDuringMouseDown);
			TestEqual(
				TEXT("Gunner does not lock the cursor to the game viewport"),
				TestViewportClient->GetMouseLockMode(),
				EMouseLockMode::DoNotLock);
			PlayerController->SetControlRotation(FRotator(0.0f, 90.0f, 0.0f));

			const int32 InitialAmmo = Gunner->GetCurrentAmmo();
			const FInputActionInstance FireActionInstance(FireStartedBinding->GetAction());
			FireStartedBinding->Execute(FireActionInstance);
			TestEqual(
				TEXT("Q does not fire before Gunner faces the aim yaw"),
				Gunner->GetCurrentAmmo(),
				InitialAmmo);

			// Exercise a quick tap directly through the bindings. Releasing Q must
			// not discard the shot that is still waiting for body alignment.
			FireCompletedBinding->Execute(FireActionInstance);
			TestEqual(
				TEXT("Quick Q release keeps the pending aligned shot"),
				Gunner->GetCurrentAmmo(),
				InitialAmmo);

			Gunner->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));
			Gunner->Tick(1.0f / 60.0f);
			const int32 SpentRounds = InitialAmmo - Gunner->GetCurrentAmmo();
			TestTrue(TEXT("Aligned Q attack consumes one or three rounds"), SpentRounds == 1 || SpentRounds == 3);
			if (Movement)
			{
				TestFalse(TEXT("Quick Q tap exits controller-facing rotation after firing"), Movement->bUseControllerDesiredRotation);
				TestTrue(TEXT("Quick Q tap restores movement-facing rotation after firing"), Movement->bOrientRotationToMovement);
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
