#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/MineLearningPlayerController.h"
#include "MineLearning/Demo/DemoRunComponent.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"

class FOreBuddySprintCheck : public IAutomationLatentCommand
{
public:
	explicit FOreBuddySprintCheck(FAutomationTestBase* InTest) : Test(InTest) {}
	bool Update() override
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
		}
		if (!World) { Test->AddError(TEXT("Sprint PIE missing")); return true; }
		AMineLearningPlayerController* PC = Cast<AMineLearningPlayerController>(World->GetFirstPlayerController());
		const float Now = World->GetTimeSeconds();
		if (Stage == 0)
		{
			PC->ExecuteDemoCommand(EDemoCommand::Start);
			AMiningCompanionCharacter* Buddy = Cast<AMiningCompanionCharacter>(PC->GetPawn());
			if (!Test->TestNotNull(TEXT("Player OreBuddy"), Buddy)) { return true; }
			// An isolated PIE-only floor removes terrain/obstacles from a movement test.
			AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>();
			Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			Floor->SetActorScale3D(FVector(100, 100, 1));
			Floor->SetActorLocation(FVector(0, 0, 2000));
			Buddy->SetActorLocation(FVector(0, 0, 2200));
			Baseline = Buddy->GetCharacterMovement()->MaxWalkSpeed;
			TestTracks(Buddy);
			Stage = 1; Until = Now + 2.f; return false;
		}
		AMiningCompanionCharacter* Buddy = Cast<AMiningCompanionCharacter>(PC->GetPawn());
		if (!Buddy) { Test->AddError(TEXT("Lost sprint pawn")); return true; }
		UCharacterMovementComponent* Movement = Buddy->GetCharacterMovement();
		if (Stage >= 3) { Buddy->AddMovementInput(FVector::ForwardVector); }
		if (Stage == 1 && Now >= Until)
		{
			Key(PC, IE_Pressed); Stage = 2; Until = Now + 0.6f;
		}
		else if (Stage == 2 && Now >= Until)
		{
			Test->TestEqual(TEXT("Idle Shift does not drain"), Buddy->GetSprintStamina(), 1.f);
			Stage = 3; Until = Now + 6.f;
		}
		else if (Stage == 3)
		{
			if (Buddy->IsSprinting())
			{
				Test->TestTrue(TEXT("Speed ramps instead of snapping"), Movement->MaxWalkSpeed > Baseline && Movement->MaxWalkSpeed < Baseline * 1.8f);
				Started = Now; Stage = 4;
			}
			else if (Now > Until) { Test->AddError(TEXT("Held Shift failed to sprint")); return true; }
		}
		else if (Stage == 4 && Now - Started > 0.6f)
		{
			Test->TestNearlyEqual(TEXT("Reached sprint speed"), Movement->MaxWalkSpeed, Baseline * 1.8f, 1.f);
			Test->TestTrue(TEXT("Stamina draining"), Buddy->GetSprintStamina() < 0.9f && Buddy->GetSprintStamina() > 0.65f);
			Stage = 5;
		}
		else if (Stage == 5 && Buddy->GetSprintStamina() <= 0.f)
		{
			Test->TestTrue(TEXT("Full charge lasts three seconds"), FMath::Abs(Now - Started - 3.f) < FMath::Max(0.15f, World->GetDeltaSeconds() * 2.f));
			Test->AddInfo(FString::Printf(TEXT("Full-charge sprint %.3f seconds"), Now - Started));
			Test->TestFalse(TEXT("Exhaustion stops boost"), Buddy->IsSprinting());
			Test->TestTrue(TEXT("Exhaustion decelerates smoothly"), Movement->MaxWalkSpeed > Baseline);
			Stage = 6; Until = Now + 2.f;
		}
		else if (Stage == 6 && Now >= Until)
		{
			Test->TestFalse(TEXT("Held exhausted Shift cannot pulse sprint"), Buddy->IsSprinting());
			Test->TestNearlyEqual(TEXT("Speed returned to normal"), Movement->MaxWalkSpeed, Baseline, 0.1f);
			Key(PC, IE_Released); Stage = 7; Until = Now + 4.f;
		}
		else if (Stage == 7 && Now >= Until)
		{
			Test->TestNearlyEqual(TEXT("Stamina recovered"), Buddy->GetSprintStamina(), 1.f, 0.001f);
			Key(PC, IE_Pressed); Stage = 8; Until = Now + 0.6f;
		}
		else if (Stage == 8 && Now >= Until)
		{
			Test->TestTrue(TEXT("Sprint available again"), Buddy->IsSprinting());
			PC->ToggleDemoTerminal(); Stage = 9; Until = Now + 0.6f;
		}
		else if (Stage == 9 && Now >= Until)
		{
			Test->TestFalse(TEXT("Menu cancels sprint"), Buddy->IsSprinting());
			Test->TestNearlyEqual(TEXT("Menu restores speed"), Movement->MaxWalkSpeed, Baseline, 0.1f);
			Key(PC, IE_Released);
			return true;
		}
		if (Started > 0.f && Now - Started > 20.f) { Key(PC, IE_Released); Test->AddError(TEXT("Sprint test stalled")); return true; }
		return false;
	}
private:
	static void Key(APlayerController* PC, EInputEvent Event)
	{
		FInputKeyEventArgs Args;
		Args.Key = EKeys::LeftShift; Args.Event = Event; Args.AmountDepressed = Event == IE_Pressed ? 1.f : 0.f;
		PC->PlayerInput->InputKey(Args);
	}
	void TestTracks(AMiningCompanionCharacter* Buddy)
	{
		UFunction* Update = Buddy->FindFunction(TEXT("UpdateTrackScroll"));
		FNumericProperty* Offset = FindFProperty<FNumericProperty>(Buddy->GetClass(), TEXT("TrackOffset_L"));
		if (!Test->TestNotNull(TEXT("Track update function"), Update) || !Test->TestNotNull(TEXT("Track offset"), Offset)) { return; }
		UCharacterMovementComponent* Movement = Buddy->GetCharacterMovement();
		Buddy->SetActorRotation(FRotator::ZeroRotator);
		FStructOnScope Params(Update);
		FNumericProperty* Delta = FindFProperty<FNumericProperty>(Update, TEXT("DeltaSeconds"));
		if (!Test->TestNotNull(TEXT("Track delta parameter"), Delta)) { return; }
		Delta->SetFloatingPointPropertyValue(Delta->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), 0.1);
		const auto ReadOffset = [&]() { return static_cast<float>(Offset->GetFloatingPointPropertyValue(Offset->ContainerPtrToValuePtr<void>(Buddy))); };
		// Establish the new heading before measuring straight-line UV travel.
		Movement->StopMovementImmediately();
		Buddy->ProcessEvent(Update, Params.GetStructMemory());
		const float Before = ReadOffset();
		Movement->Velocity = FVector(200,0,0);
		Buddy->ProcessEvent(Update, Params.GetStructMemory());
		const float NormalDelta = ReadOffset() - Before;
		Movement->MaxWalkSpeed = Baseline * 1.8f;
		const float SprintBefore = ReadOffset();
		Buddy->ProcessEvent(Update, Params.GetStructMemory());
		Test->TestNearlyEqual(TEXT("Same velocity has same UV speed regardless of speed limit"), ReadOffset() - SprintBefore, NormalDelta, 0.0001f);
		Movement->Velocity = FVector(360,0,0);
		const float FastBefore = ReadOffset();
		Buddy->ProcessEvent(Update, Params.GetStructMemory());
		Test->TestNearlyEqual(TEXT("UV scales with actual sprint velocity"), ReadOffset() - FastBefore, NormalDelta * 1.8f, 0.0001f);
		Movement->Velocity = FVector(-200,0,0);
		const float ReverseBefore = ReadOffset();
		Buddy->ProcessEvent(Update, Params.GetStructMemory());
		Test->TestNearlyEqual(TEXT("Reverse reverses track UV"), ReadOffset() - ReverseBefore, -NormalDelta, 0.0001f);
		Movement->StopMovementImmediately(); Movement->MaxWalkSpeed = Baseline;
	}
	FAutomationTestBase* Test;
	int32 Stage = 0;
	float Until = 0.f, Started = -1.f, Baseline = 0.f;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOreBuddySprintTest, "MineLearning.OreBuddy.SprintAndTracks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOreBuddySprintTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FOreBuddySprintCheck(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
