#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/AttackStackEffect.h"
#include "MineLearning/Combat/WeaponRecoilComponent.h"
#include "MineLearning/Presentation/CombatFeedbackComponent.h"
#include "MineLearning/Mining/MineableOre.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"

class FCombatFeedbackCheck : public IAutomationLatentCommand
{
public:
	explicit FCombatFeedbackCheck(FAutomationTestBase* InTest) : Test(InTest) {}
	bool Update() override
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
		}
		if (!Test->TestNotNull(TEXT("PIE world"), World)) { return true; }
		if (!Gunner.IsValid())
		{
			APlayerController* Player = World->GetFirstPlayerController();
			if (!Test->TestNotNull(TEXT("PIE player"), Player)) { return true; }
			AMiningCompanionCharacter* Buddy = World->SpawnActor<AMiningCompanionCharacter>();
			Player->Possess(Buddy);
			Player->SetControlRotation(FRotator(-10.f, 30.f, 0.f));
			Buddy->FindComponentByClass<USpringArmComponent>()->TickComponent(0.016f, LEVELTICK_All, nullptr);
			UCameraComponent* Camera = Buddy->FindComponentByClass<UCameraComponent>();
			const FTransform Before = Camera->GetComponentTransform();
			Buddy->SetActorRotation(FRotator(0.f, 150.f, 0.f));
			Test->TestTrue(TEXT("OreBuddy body turn does not perturb camera between ticks"), Before.Equals(Camera->GetComponentTransform(), 0.01f));
			Player->UnPossess();
			Buddy->Destroy();
			UClass* Class = LoadClass<AGunnerCharacter>(nullptr, TEXT("/Game/MineLearning/Characters/Gunner/Blueprints/BP_Gunner.BP_Gunner_C"));
			Gunner = World->SpawnActor<AGunnerCharacter>(Class, FVector(0, 0, 300), FRotator::ZeroRotator);
			if (!Test->TestNotNull(TEXT("Gunner"), Gunner.Get())) { return true; }
			Player->Possess(Gunner.Get());
			Player->SetControlRotation(FRotator::ZeroRotator);
			Gunner->GetCharacterMovement()->DisableMovement();
			Effects = NewObject<UUnitEffectComponent>(Gunner.Get());
			Gunner->AddInstanceComponent(Effects);
			Effects->RegisterComponent();
			Feedback = NewObject<UCombatFeedbackComponent>(Gunner.Get());
			Gunner->AddInstanceComponent(Feedback);
			Feedback->RegisterComponent();
			UAttackStackEffectDefinition* Rule = LoadObject<UAttackStackEffectDefinition>(nullptr, TEXT("/Game/MineLearning/GameplayRuntime/Effects/DA_Roamer.DA_Roamer"));
			if (!Test->TestNotNull(TEXT("Configured Roamer"), Rule)) { return true; }
			Test->TestFalse(TEXT("Roamer rejects non-Gunner even via direct grant"), Rule->SupportsTarget(Player));
			Test->TestTrue(TEXT("Grant configured Gunner effect"), Effects->GrantDefinition(TEXT("Roamer"), Rule));
			UCombatComponent* Combat = Gunner->FindComponentByClass<UCombatComponent>();
			Gunner->GetCharacterMovement()->Velocity = FVector(0, 100, 0);
			Combat->NotifyAttackResolved(true);
			if (!Test->TestEqual(TEXT("Active Roamer"), Effects->GetActiveEffectViews().Num(), 1)) { return true; }
			Test->TestEqual(TEXT("Strafe hit grants two layers"), Effects->GetActiveEffectViews()[0].StackCount, 2);
			const float LowAura = Feedback->GetAuraIntensity();
			Gunner->GetCharacterMovement()->Velocity = FVector(100, 0, 0);
			Combat->NotifyAttackResolved(true);
			Test->TestEqual(TEXT("Forward hit adds one layer"), Effects->GetActiveEffectViews()[0].StackCount, 3);
			Test->TestTrue(TEXT("Aura intensity grows with stacks"), Feedback->GetAuraIntensity() > LowAura);
			Test->TestNotNull(TEXT("Aura material attached"), Gunner->GetMesh()->GetOverlayMaterial());
			Test->TestNotNull(TEXT("Active buff carries its icon"), Effects->GetActiveEffectViews()[0].Icon.Get());
			Combat->NotifyAttackResolved(false, false);
			Test->TestEqual(TEXT("Miss clears aura immediately"), Feedback->GetAuraIntensity(), 0.f);
			Gunner->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			AMineableOre* Ore = World->SpawnActor<AMineableOre>(FVector(2000, 0, 300), FRotator::ZeroRotator);
			Gunner->RestoreLoadedAmmo(20);
			Test->TestTrue(TEXT("Out-of-range shot emitted"), Gunner->TryFireAtOre(Ore));
			Test->TestEqual(TEXT("Out-of-range still spends ammo"), Gunner->GetCurrentAmmo(), 19);
			Test->TestTrue(TEXT("Actual rejected shot shows range ring"), Feedback->IsRangeVisible());
			Ore->Destroy();
			UWeaponRecoilComponent* Recoil = Gunner->FindComponentByClass<UWeaponRecoilComponent>();
			Recoil->RandomSpreadDegrees = 0.f;
			FVector First = FVector::ZeroVector;
			FVector Last = FVector::ZeroVector;
			for (int32 I = 0; I < 10; ++I)
			{
				Last = Recoil->ApplyShot(FVector::ForwardVector);
				if (I == 0) { First = Last; }
				Test->TestTrue(TEXT("Spray remains above aimed direction"), Last.Z >= 0.f);
			}
			Test->TestTrue(TEXT("Sustained recoil climbs and turns left"), Last.Z > First.Z && Last.Y < First.Y);
			Test->TestTrue(TEXT("Recoil expands crosshair"), Recoil->GetCrosshairScale().X > 2.f);
			Until = World->GetTimeSeconds() + 2.15f;
			return false;
		}
		if (World->GetTimeSeconds() < Until) { return false; }
		Test->TestFalse(TEXT("Range ring hides after two seconds"), Feedback->IsRangeVisible());
		Test->TestEqual(TEXT("Idle recoil fully recovers"), Gunner->FindComponentByClass<UWeaponRecoilComponent>()->GetCrosshairScale().X, 1.);
		Gunner->Destroy();
		return true;
	}
private:
	FAutomationTestBase* Test;
	TWeakObjectPtr<AGunnerCharacter> Gunner;
	UUnitEffectComponent* Effects = nullptr;
	UCombatFeedbackComponent* Feedback = nullptr;
	float Until = 0.f;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatFeedbackTest, "MineLearning.Roguelite.CombatFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatFeedbackTest::RunTest(const FString& Parameters)
{
	FAutomationEditorCommonUtils::CreateNewMap();
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCombatFeedbackCheck(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
