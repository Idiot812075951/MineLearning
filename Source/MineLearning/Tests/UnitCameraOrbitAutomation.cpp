#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "HAL/IConsoleManager.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/MineLearningPlayerController.h"
#include "MineLearning/Demo/DemoRunComponent.h"

/** Samples the rendered camera after the real movement, spring-arm and camera-manager ticks. */
class FUnitCameraOrbitCheck : public IAutomationLatentCommand
{
public:
	explicit FUnitCameraOrbitCheck(FAutomationTestBase* InTest) : Test(InTest) {}
	~FUnitCameraOrbitCheck()
	{
		if (FrameRateLimit)
		{
			FrameRateLimit->SetWithCurrentPriority(PreviousFrameRateLimit);
		}
		if (Player.IsValid() && Player->PlayerInput)
		{
			SendKey(EKeys::W, IE_Released);
			SendKey(EKeys::RightMouseButton, IE_Released);
		}
		FWorldDelegates::OnWorldPreActorTick.Remove(PreTick);
		FWorldDelegates::OnWorldPostActorTick.Remove(PostTick);
	}
	bool Update() override
	{
		if (!World.IsValid())
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
			}
			if (!Test->TestNotNull(TEXT("Orbit PIE world"), World.Get())) { return true; }
			Player = Cast<AMineLearningPlayerController>(World->GetFirstPlayerController());
			if (!Test->TestNotNull(TEXT("Gameplay controller"), Player.Get())) { return true; }
			Player->ExecuteDemoCommand(EDemoCommand::Start);
			Player->CloseRogueliteMenu();
			Test->TestFalse(TEXT("Movement is enabled during orbit"), Player->IsMoveInputIgnored());
			Buddy = Cast<AMiningCompanionCharacter>(Player->GetPawn());
			if (!Test->TestNotNull(TEXT("Shipped OreBuddy blueprint"), Buddy.Get())) { return true; }
			AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>();
			Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			Floor->SetActorScale3D(FVector(100, 100, 1));
			Floor->SetActorLocation(FVector(0, 0, 2000));
			Buddy->SetActorLocation(FVector(0, 0, 2200));
			PreviousPosition = Buddy->GetActorLocation();
			Boom = Buddy->FindComponentByClass<USpringArmComponent>();
			Camera = Buddy->FindComponentByClass<UCameraComponent>();
			Test->TestTrue(TEXT("Camera yaw independent from body"), Boom->IsUsingAbsoluteRotation());
			Test->TestFalse(TEXT("No location lag"), Boom->bEnableCameraLag);
			Test->TestFalse(TEXT("No rotation lag"), Boom->bEnableCameraRotationLag);
			PhaseStarted = World->GetTimeSeconds();
			FrameRateLimit = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"));
			PreviousFrameRateLimit = FrameRateLimit->GetFloat();
			Player->SetControlRotation(FRotator(-10.f, 0.f, 0.f));
			SendKey(EKeys::W, IE_Pressed);
			SendKey(EKeys::RightMouseButton, IE_Pressed);
			PreTick = FWorldDelegates::OnWorldPreActorTick.AddRaw(this, &FUnitCameraOrbitCheck::BeforeTick);
			PostTick = FWorldDelegates::OnWorldPostActorTick.AddRaw(this, &FUnitCameraOrbitCheck::AfterTick);
			return false;
		}
		if (World->GetTimeSeconds() - PhaseStarted < 3.f) { return false; }
		Test->AddInfo(FString::Printf(TEXT("Orbit rate=%g body rate=%g: %d frames, camera angle error=%.5f deg, camera offset error=%.5f cm, rendered view error=%.5f cm, body heading lag=%.2f deg"),
			OrbitRate, Buddy->GetCharacterMovement()->RotationRate.Yaw, Samples, MaxAngleError, MaxOffsetError, MaxViewError, MaxBodyLag));
		Test->TestTrue(TEXT("Enough actual moving frames sampled"), Samples > 15);
		Test->TestTrue(TEXT("Character really moves during probe"), TravelDistance > 100.f);
		Test->TestTrue(TEXT("Movement and turning do not alter camera angle"), MaxAngleError < 0.02f);
		Test->TestTrue(TEXT("Camera position matches current-frame pawn and aim"), MaxOffsetError < 0.05f);
		Test->TestTrue(TEXT("Rendered camera uses current spring-arm pose"), MaxViewError < 0.05f);
		Test->TestTrue(TEXT("Camera-relative movement uses the current view"), MaxMovementHeadingError < 0.02f);
		if (Phase == 0)
		{
			Test->TestTrue(TEXT("Normal turning does not wobble behind the camera at variable frame rates"), MaxBodyLag < 0.02f);
		}
		if (++Phase == 3) { return true; }
		// Exaggerate the user's hypothesis, then orbit faster than normal body rotation.
		Buddy->GetCharacterMovement()->RotationRate.Yaw = Phase == 1 ? 60.f : 500.f;
		OrbitRate = Phase == 2 ? 900.f : 180.f;
		Samples = 0; MaxAngleError = MaxOffsetError = MaxViewError = MaxBodyLag = 0.f;
		TravelDistance = 0.f;
		MaxMovementHeadingError = 0.f;
		PhaseStarted = World->GetTimeSeconds();
		return false;
	}
private:
	void SendKey(FKey Key, EInputEvent Event)
	{
		Player->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), Key, Event, FPlatformTime::Cycles64()));
	}
	void BeforeTick(UWorld* TickWorld, ELevelTick TickType, float DeltaSeconds)
	{
		if (TickWorld != World.Get() || !Buddy.IsValid()) { return; }
		FrameRateLimit->SetWithCurrentPriority(static_cast<int32>(World->GetTimeSeconds() * 4.f) % 2 == 0 ? 30.f : 120.f);
		// Go through Enhanced Input, movement callbacks and controller UpdateRotation.
		Player->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), EKeys::MouseX,
			OrbitRate * DeltaSeconds / 0.07f, DeltaSeconds, 1, FPlatformTime::Cycles64()));
	}
	void AfterTick(UWorld* TickWorld, ELevelTick TickType, float DeltaSeconds)
	{
		if (TickWorld != World.Get() || !Buddy.IsValid() || World->GetTimeSeconds() - PhaseStarted < 0.3f) { return; }
		const FRotator View = Player->GetControlRotation();
		const UCharacterMovementComponent* Movement = Buddy->GetCharacterMovement();
		if (!Movement->GetCurrentAcceleration().IsNearlyZero())
		{
			MaxMovementHeadingError = FMath::Max(MaxMovementHeadingError,
				FMath::Abs(FMath::FindDeltaAngleDegrees(Movement->GetCurrentAcceleration().Rotation().Yaw, View.Yaw)));
		}
		TravelDistance += FVector::Distance(PreviousPosition, Buddy->GetActorLocation());
		PreviousPosition = Buddy->GetActorLocation();
		const FVector Expected = Buddy->GetActorLocation() + Boom->TargetOffset - View.Vector() * Boom->TargetArmLength
			+ View.RotateVector(Boom->SocketOffset);
		MaxAngleError = FMath::Max(MaxAngleError, static_cast<float>((Camera->GetComponentRotation() - View).GetNormalized().GetManhattanDistance(FRotator::ZeroRotator)));
		MaxOffsetError = FMath::Max(MaxOffsetError, static_cast<float>(FVector::Distance(Camera->GetComponentLocation(), Expected)));
		MaxViewError = FMath::Max(MaxViewError, static_cast<float>(FVector::Distance(Player->PlayerCameraManager->GetCameraLocation(), Camera->GetComponentLocation())));
		MaxBodyLag = FMath::Max(MaxBodyLag, FMath::Abs(FMath::FindDeltaAngleDegrees(Buddy->GetActorRotation().Yaw, View.Yaw)));
		++Samples;
	}
	FAutomationTestBase* Test;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AMineLearningPlayerController> Player;
	TWeakObjectPtr<AMiningCompanionCharacter> Buddy;
	USpringArmComponent* Boom = nullptr;
	UCameraComponent* Camera = nullptr;
	FDelegateHandle PreTick, PostTick;
	float PhaseStarted = 0.f;
	float OrbitRate = 180.f;
	float MaxAngleError = 0.f, MaxOffsetError = 0.f, MaxViewError = 0.f, MaxBodyLag = 0.f;
	float TravelDistance = 0.f;
	float MaxMovementHeadingError = 0.f;
	IConsoleVariable* FrameRateLimit = nullptr;
	float PreviousFrameRateLimit = 0.f;
	FVector PreviousPosition = FVector::ZeroVector;
	int32 Samples = 0;
	int32 Phase = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitCameraOrbitTest, "MineLearning.PlayerForm.OreBuddyCameraOrbit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUnitCameraOrbitTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FUnitCameraOrbitCheck(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
