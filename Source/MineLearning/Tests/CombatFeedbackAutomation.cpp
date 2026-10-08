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
#include "MineLearning/Mining/MiningToolComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

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
			UClass* BuddyClass = LoadClass<AMiningCompanionCharacter>(nullptr, TEXT("/Game/MineLearning/Characters/OreBuddy/Blueprints/BP_OreBuddy07.BP_OreBuddy07_C"));
			AMiningCompanionCharacter* Buddy = World->SpawnActor<AMiningCompanionCharacter>(BuddyClass, FVector(0, 0, 300), FRotator::ZeroRotator);
			Player->Possess(Buddy);
			UCombatComponent* BuddyCombat = Buddy->FindComponentByClass<UCombatComponent>();
			AMineableOre* SideOre = World->SpawnActor<AMineableOre>(FVector(0, 100, 300), FRotator::ZeroRotator);
			Test->TestEqual(TEXT("Melee acquires target beside current facing"), BuddyCombat->FindNearestAttackTarget(), static_cast<AActor*>(SideOre));
			Buddy->TryUseMiningSkill();
			Test->TestTrue(TEXT("Mining turns toward in-range target"), FMath::IsNearlyEqual(Buddy->GetActorRotation().Yaw, 90.f, 0.1f));
			UMiningToolComponent* Mining = Buddy->FindComponentByClass<UMiningToolComponent>();
			Mining->CancelMining();
			SideOre->SetActorLocation(FVector(0, 10000, 300));
			Test->TestNull(TEXT("Melee never acquires outside current reach"), BuddyCombat->FindNearestAttackTarget());
			Buddy->TryUseMiningSkill();
			Test->TestTrue(TEXT("No target still plays complete mining swing"), Mining->IsMining());
			Test->TestTrue(TEXT("Empty swing preserves facing"), FMath::IsNearlyEqual(Buddy->GetActorRotation().Yaw, 90.f, 0.1f));
			Mining->CancelMining();
			SideOre->Destroy();
			UUnitEffectComponent* BuddyEffects = NewObject<UUnitEffectComponent>(Buddy);
			Buddy->AddInstanceComponent(BuddyEffects); BuddyEffects->RegisterComponent();
			UCombatFeedbackComponent* BuddyFeedback = NewObject<UCombatFeedbackComponent>(Buddy);
			Buddy->AddInstanceComponent(BuddyFeedback); BuddyFeedback->RegisterComponent();
			TArray<UMaterialInterface*> OriginalMaterials = Buddy->GetMesh()->GetMaterials();
			FUnitEffectRule ChargeRule; ChargeRule.Id = TEXT("ToolVisualTest"); ChargeRule.bToolGlow = true; ChargeRule.AuraColor = FLinearColor(0.f, 0.8f, 1.f);
			BuddyEffects->ApplyEffect(ChargeRule);
			const int32 DrillSlot = Buddy->GetMesh()->GetMaterialIndex(TEXT("MAT_OB07_ToolMetal"));
			UMaterialInstanceDynamic* DrillMaterial = DrillSlot != INDEX_NONE ? Cast<UMaterialInstanceDynamic>(Buddy->GetMesh()->GetMaterial(DrillSlot)) : nullptr;
			Test->TestNotNull(TEXT("Charged drill uses its own dynamic material"), DrillMaterial);
			if (DrillMaterial) { Test->TestEqual(TEXT("Drill charge material enabled"), DrillMaterial->K2_GetScalarParameterValue(TEXT("ChargeIntensity")), 1.f); }
			for (int32 Slot = 0; Slot < OriginalMaterials.Num(); ++Slot)
			{
				if (Slot != DrillSlot) { Test->TestEqual(TEXT("Charging preserves every body material"), Buddy->GetMesh()->GetMaterial(Slot), OriginalMaterials[Slot]); }
			}
			Test->TestNull(TEXT("Tool charge never installs whole-body overlay"), Buddy->GetMesh()->GetOverlayMaterial());
			BuddyEffects->RemoveEffect(TEXT("ToolVisualTest"));
			if (DrillMaterial) { Test->TestEqual(TEXT("Charge material clears when effect ends"), DrillMaterial->K2_GetScalarParameterValue(TEXT("ChargeIntensity")), 0.f); }
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
			FCombatModifiers RangeBonus;
			RangeBonus.AttackRange = 0.5f;
			Combat->SetModifier(TEXT("RangeTest"), RangeBonus);
			Test->TestTrue(TEXT("Range change immediately shows ring"), Feedback->IsRangeVisible());
			Combat->RemoveModifier(TEXT("RangeTest"));
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
