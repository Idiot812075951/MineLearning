#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "MineLearning/MineLearningPlayerController.h"
#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/Demo/DemoRunComponent.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/Mining/ResourceStorageComponent.h"
#include "MineLearning/UI/GunnerHitFeedbackComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

class FCombatUIFixCheck : public IAutomationLatentCommand
{
public:
	explicit FCombatUIFixCheck(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
		}
		if (!World) { Test->AddError(TEXT("PIE world missing")); return true; }
		AMineLearningPlayerController* PC = Cast<AMineLearningPlayerController>(World->GetFirstPlayerController());
		if (!PC) { Test->AddError(TEXT("Demo controller missing")); return true; }
		UDemoRunComponent* Run = PC->GetDemoRun();
		if (Step == 0)
		{
			Test->TestTrue(TEXT("GM works directly from briefing"), Run->ExecuteCommand(EDemoCommand::UnlockAll));
			Test->TestEqual(TEXT("GM starts production"), Run->GetPhase(), EDemoPhase::Production);
			Test->TestTrue(TEXT("Gunner unlocked"), Run->IsFormUnlocked(EPlayerTransformationForm::Gunner));
			Test->TestTrue(TEXT("Guren unlocked"), Run->IsFormUnlocked(EPlayerTransformationForm::Guren));
			Test->TestEqual(TEXT("Strength maximum"), Run->CoreBonus.Strength, 15.f);
			Test->TestEqual(TEXT("Agility maximum"), Run->CoreBonus.Agility, 15.f);
			Test->TestEqual(TEXT("Intelligence maximum"), Run->CoreBonus.Intelligence, 15.f);
			AWarehouseDepot* Warehouse = *TActorIterator<AWarehouseDepot>(World);
			if (!Warehouse) { Test->AddError(TEXT("Warehouse missing")); return true; }
			UResourceStorageComponent* Storage = Warehouse->GetStorageComponent();
			for (EItemType Type : {EItemType::IronOre, EItemType::IronIngot, EItemType::Coin, EItemType::Ammo})
			{
				Test->TestTrue(TEXT("All resource types funded"), Storage->GetAvailableItemAmount(Type) >= 10000);
			}
			Storage->TryReserveItem({EItemType::IronOre, 4});
			Test->TestTrue(TEXT("Repeated GM succeeds"), Run->ExecuteCommand(EDemoCommand::UnlockAll));
			Test->TestEqual(TEXT("GM preserves reservations"), Storage->GetReservedItemAmount(EItemType::IronOre), 4);
			Test->TestEqual(TEXT("GM tops up available ore"), Storage->GetAvailableOre(), 10000);
			Test->TestTrue(TEXT("Further production still fits"), Storage->AddItem({EItemType::IronIngot, 1}));
			Test->TestEqual(TEXT("Magazines topped up without repeated addition"), Run->GetReserveMagazines(), 999);
			Test->TestTrue(TEXT("Gunner available immediately"), Run->ExecuteCommand(EDemoCommand::Gunner));
			Gunner = Cast<AGunnerCharacter>(PC->GetPawn());
			if (!Gunner.IsValid()) { Test->AddError(TEXT("Gunner missing")); return true; }
			Test->TestEqual(TEXT("Gunner magazine full"), Gunner->GetCurrentAmmo(), Gunner->GetMagazineSize());
			Test->TestNotNull(TEXT("Independent feedback host installed"), Gunner->FindComponentByClass<UGunnerHitFeedbackComponent>());
			const FVector Top = Warehouse->GetActorLocation() + FVector(0.f, 0.f, 200.f);
			Gunner->OnCriticalHit.Broadcast(false, Warehouse, Top);
			UWidgetComponent* Host = FindFeedback();
			if (!Host) { Test->AddError(TEXT("Critical feedback widget missing")); return true; }
			Test->TestTrue(TEXT("Badge anchored above victim"), Host->GetComponentLocation().Equals(Top + FVector(0.f, 0.f, 24.f), 1.f));
			Test->TestEqual(TEXT("Badge follows victim root"), Host->GetAttachParent(), Warehouse->GetRootComponent());
			Test->TestEqual(TEXT("Badge uses camera-independent world anchor"), Host->GetWidgetSpace(), EWidgetSpace::Screen);
			Test->TestFalse(TEXT("Silver input"), CastChecked<UGunnerHitFeedbackWidget>(Host->GetUserWidgetObject())->bGoldenHit);
			// A lethal hit still has a pre-damage position even when the target no longer exists.
			Gunner->OnCriticalHit.Broadcast(true, nullptr, Top);
			Host = FindFeedback();
			if (!Host) { Test->AddError(TEXT("Lethal feedback missing")); return true; }
			Test->TestTrue(TEXT("Golden input"), CastChecked<UGunnerHitFeedbackWidget>(Host->GetUserWidgetObject())->bGoldenHit);
			Test->TestTrue(TEXT("Lethal hit retains target location"), Host->GetComponentLocation().Equals(Top + FVector(0.f, 0.f, 24.f), 1.f));
			WaitUntil = World->GetTimeSeconds() + 1.f;
			++Step;
			return false;
		}
		if (World->GetTimeSeconds() < WaitUntil) { return false; }
		Test->TestNull(TEXT("Expired feedback removed"), FindFeedback());
		Test->TestTrue(TEXT("Guren immediately accessible"), Run->ExecuteCommand(EDemoCommand::Guren));
		Test->TestTrue(TEXT("Switch back to Gunner"), Run->ExecuteCommand(EDemoCommand::Gunner));
		Test->TestEqual(TEXT("Ammo survives form changes"), CastChecked<AGunnerCharacter>(PC->GetPawn())->GetCurrentAmmo(), 20);
		return true;
	}
private:
	UWidgetComponent* FindFeedback() const
	{
		if (!Gunner.IsValid()) { return nullptr; }
		TInlineComponentArray<UWidgetComponent*> Widgets(Gunner.Get());
		for (UWidgetComponent* Widget : Widgets)
		{
			if (Cast<UGunnerHitFeedbackWidget>(Widget->GetUserWidgetObject())) { return Widget; }
		}
		return nullptr;
	}
	FAutomationTestBase* Test;
	TWeakObjectPtr<AGunnerCharacter> Gunner;
	int32 Step = 0;
	float WaitUntil = 0.f;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatUIFixTest, "MineLearning.UI.GMAndTargetFeedback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatUIFixTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/MineLearning/Maps/L_WorldLayout_P01")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCombatUIFixCheck(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
