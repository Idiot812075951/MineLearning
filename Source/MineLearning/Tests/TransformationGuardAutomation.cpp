#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "MineLearning/TransformationGuard.h"
#include "MineLearning/PlayerTransformZone.h"
#include "MineLearning/Demo/DemoRunComponent.h"
#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/Manifestation/Guren/AGurenCharacter.h"
#include "MineLearning/Manifestation/Guren/GurenQSkillComponent.h"
#include "MineLearning/Manifestation/Guren/GurenUltimateComponent.h"
#include "MineLearning/Mining/MiningToolComponent.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/ItemPickup.h"

class FTransformationGuardCheck : public IAutomationLatentCommand
{
public:
	explicit FTransformationGuardCheck(FAutomationTestBase* InTest) : Test(InTest) {}

	bool Update() override
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
		}
		if (!Test->TestNotNull(TEXT("PIE world"), World)) { return true; }
		APlayerController* PC = World->GetFirstPlayerController();
		UDemoRunComponent* Run = PC->FindComponentByClass<UDemoRunComponent>();
		const float Now = World->GetTimeSeconds();
		const TCHAR* Names[] = { TEXT("Guren primary"), TEXT("Guren Q"), TEXT("Guren R"), TEXT("Gunner fire"), TEXT("Gunner reload"), TEXT("OreBuddy mining"), TEXT("OreBuddy pickup") };
		if (Case >= UE_ARRAY_COUNT(Names)) { return true; }
		if (Phase == 0)
		{
			Run->ExecuteCommand(EDemoCommand::UnlockAll);
			// Return through the production entry point between cases to verify the lock releases.
			if (!Test->TestTrue(TEXT("Idle form can return to human"), Run->ExecuteCommand(EDemoCommand::Human))) { return true; }
			const EDemoCommand Form = Case < 3 ? EDemoCommand::Guren : Case < 5 ? EDemoCommand::Gunner : EDemoCommand::OreBuddy;
			if (!Test->TestTrue(TEXT("Can enter next test form"), Run->ExecuteCommand(Form))) { return true; }
			Pawn = PC->GetPawn();
			Pawn->SetActorLocation(FVector(800, -1300, 120));
			Pawn->SetActorRotation(FRotator(0, 90, 0));
			if (Case == 1 || Case == 2 || Case == 5)
			{
				UClass* OreClass = LoadClass<AMineableOre>(nullptr, TEXT("/Game/MineLearning/Mining/Ores/Iron/Blueprints/BP_Ore_Stone.BP_Ore_Stone_C"));
				World->SpawnActor<AMineableOre>(OreClass, FVector(800, -1180, 70), FRotator::ZeroRotator);
			}
			if (Case == 6)
			{
				AItemPickup* Pickup = World->SpawnActor<AItemPickup>(Pawn->GetActorLocation() + FVector(0, 70, -40), FRotator::ZeroRotator);
				FItemStack Stack;
				Stack.ItemType = EItemType::IronOre;
				Stack.Amount = 1;
				Pickup->SetItemStack(Stack);
			}
			Phase = 1;
			Until = Now + 1.f;
			return false;
		}
		if (!Test->TestTrue(TEXT("Action keeps its original pawn"), Pawn.IsValid() && PC->GetPawn() == Pawn.Get())) { return true; }
		ITransformationGuard* Guard = Cast<ITransformationGuard>(Pawn.Get());
		if (!Test->TestNotNull(TEXT("Production form implements transformation guard"), Guard)) { return true; }
		if (Phase == 1)
		{
			if (Now < Until) { return false; }
			if (Case == 0) { CastChecked<AGurenCharacter>(Pawn.Get())->TryPrimaryAttack(); }
			else if (Case == 1) { CastChecked<AGurenCharacter>(Pawn.Get())->QSkill->TryCast(); }
			else if (Case == 2) { CastChecked<AGurenCharacter>(Pawn.Get())->Ultimate->TryStart(); }
			else if (Case == 3)
			{
				AGunnerCharacter* Gunner = CastChecked<AGunnerCharacter>(Pawn.Get());
				Gunner->TryFireAtAim(Gunner->GetActorLocation() + FVector(0, 0, 100), FVector::ForwardVector);
			}
			else if (Case == 4)
			{
				AGunnerCharacter* Gunner = CastChecked<AGunnerCharacter>(Pawn.Get());
				Gunner->RestoreLoadedAmmo(1);
				Gunner->RequestReload();
			}
			else if (Case == 5) { CastChecked<AMiningCompanionCharacter>(Pawn.Get())->TryUseMiningSkill(); }
			else { CastChecked<AMiningCompanionCharacter>(Pawn.Get())->TryUsePickupSkill(); }
			if (!Test->TestFalse(FString::Printf(TEXT("%s starts and blocks transformation"), Names[Case]), Guard->CanTransform())) { return true; }
			Phase = 2;
			Until = Now + 30.f;
		}
		if (Guard->CanTransform())
		{
			Test->AddInfo(FString::Printf(TEXT("%s completed; transformation unlocked"), Names[Case]));
			++Case;
			Phase = 0;
			if (Case == UE_ARRAY_COUNT(Names))
			{
				Test->TestTrue(TEXT("Final completed action allows actual swap"), Run->ExecuteCommand(EDemoCommand::Human));
				return true;
			}
			return false;
		}
		// Both terminal and direct zone calls must reject throughout the whole action,
		// including recovery, without destroying the pawn or cancelling its action.
		Test->TestFalse(TEXT("Terminal rejects busy form"), Run->ExecuteCommand(EDemoCommand::Human));
		for (TActorIterator<APlayerTransformZone> It(World); It; ++It)
		{
			Test->TestFalse(TEXT("Direct zone rejects busy form"), It->TrySelectForm(PC, EPlayerTransformationForm::Human));
		}
		Test->TestTrue(TEXT("Rejected transformation preserves pawn"), PC->GetPawn() == Pawn.Get());
		if (Now > Until) { Test->AddError(FString::Printf(TEXT("%s never released transformation guard"), Names[Case])); return true; }
		return Test->HasAnyErrors();
	}
private:
	FAutomationTestBase* Test;
	TWeakObjectPtr<APawn> Pawn;
	int32 Case = 0;
	int32 Phase = 0;
	float Until = 0.f;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransformationGuardTest, "MineLearning.Transformation.ActiveActions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTransformationGuardTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FTransformationGuardCheck(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
