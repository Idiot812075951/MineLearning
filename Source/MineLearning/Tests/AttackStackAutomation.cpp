#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "MineLearning/Combat/AttackStackEffect.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/UnitMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "MineLearning/AI/GunnerCharacter.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "Engine/Engine.h"
#include "Tests/AutomationCommon.h"

class FAttackStackCheck : public IAutomationLatentCommand
{
public:
	explicit FAttackStackCheck(FAutomationTestBase* InTest) : Test(InTest) {}
	bool Update() override
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
		}
		if (!Test->TestNotNull(TEXT("PIE world"), World)) { return true; }
		if (Step == 0)
		{
			Unit = World->SpawnActor<ACharacter>();
			Unit->GetCharacterMovement()->DisableMovement();
			Combat = NewObject<UCombatComponent>(Unit);
			Unit->AddInstanceComponent(Combat);
			Combat->RegisterComponent();
			Effects = NewObject<UUnitEffectComponent>(Unit);
			Unit->AddInstanceComponent(Effects);
			Effects->RegisterComponent();
			UUnitMovementComponent* Movement = NewObject<UUnitMovementComponent>(Unit);
			Unit->AddInstanceComponent(Movement);
			Movement->RegisterComponent();
			if (!Unit->HasActorBegunPlay()) { Unit->DispatchBeginPlay(); }
			UAttackStackEffectDefinition* Conqueror = NewObject<UAttackStackEffectDefinition>();
			Conqueror->Rule.Id = TEXT("Conqueror");
			Conqueror->Rule.Modifiers.AttackSpeed = 0.05f;
			Conqueror->Rule.Modifiers.PrimaryDamage = 0.05f;
			Conqueror->FullStackModifiers.AttackRange = 0.25f;
			Test->TestTrue(TEXT("Grant stack behavior"), Effects->GrantDefinition(TEXT("Conqueror"), Conqueror));
			for (int32 Index = 0; Index < 7; ++Index) { Combat->NotifyAttackResolved(true); }
			Test->TestEqual(TEXT("Range remains unchanged below full stacks"), Combat->GetAttackRange(), 200.f);
			Combat->NotifyAttackResolved(true);
			Combat->NotifyAttackResolved(true);
			if (!Test->TestEqual(TEXT("One active stack effect"), Effects->GetActiveEffectViews().Num(), 1)) { return true; }
			Test->TestEqual(TEXT("Exactly one shared stack count, capped at eight"), Effects->GetActiveEffectViews()[0].StackCount, 8);
			Test->TestTrue(TEXT("Attack bonus from same count"), FMath::IsNearlyEqual(Combat->GetAttackSpeedScale(), 1.4f));
			Test->TestTrue(TEXT("Damage bonus from same count"), FMath::IsNearlyEqual(Combat->GetModifiers().PrimaryDamage, 0.4f));
			Test->TestEqual(TEXT("Full-stack range bonus"), Combat->GetAttackRange(), 250.f);
			Until = World->GetTimeSeconds() + 5.15f;
			Step = 1;
			return false;
		}
		if (World->GetTimeSeconds() < Until) { return false; }
		if (!Test->TestEqual(TEXT("Expired effect still has layers"), Effects->GetActiveEffectViews().Num(), 1)) { return true; }
		Test->TestEqual(TEXT("Expiry loses one stack instead of all stacks"), Effects->GetActiveEffectViews()[0].StackCount, 7);
		Test->TestEqual(TEXT("Range bonus removed immediately below max"), Combat->GetAttackRange(), 200.f);
		Combat->NotifyAttackResolved(true);
		Test->TestEqual(TEXT("Hit during decay restores a layer"), Effects->GetActiveEffectViews()[0].StackCount, 8);
		Effects->RevokeDefinition(TEXT("Conqueror"));
		Test->TestEqual(TEXT("Revocation removes all contributions"), Combat->GetAttackSpeedScale(), 1.f);
		UAttackStackEffectDefinition* Roamer = NewObject<UAttackStackEffectDefinition>();
		Roamer->Rule.Id = TEXT("Roamer");
		Roamer->Rule.Duration = 3.f;
		Roamer->MaxStacks = 0;
		Roamer->bRequiresMovement = true;
		Roamer->bClearOnMiss = true;
		Roamer->Rule.Modifiers.AttackSpeed = 0.05f;
		Roamer->Rule.Modifiers.MoveSpeed = 0.05f;
		Roamer->EquippedModifiers.AIMissChance = 1.f;
		Effects->GrantDefinition(TEXT("Roamer"), Roamer);
		Combat->NotifyAttackResolved(true);
		Test->TestTrue(TEXT("Stationary hits do not stack"), Effects->GetActiveEffectViews().IsEmpty());
		for (int32 Index = 0; Index < 100; ++Index) { Combat->OnPrimaryAttackResolved.Broadcast(true, true); }
		Test->TestEqual(TEXT("No configured stack-count limit"), Effects->GetActiveEffectViews()[0].StackCount, 100);
		Test->TestEqual(TEXT("Final attack interval respects five attacks per second"), Combat->GetAttackInterval(0.7f), 0.2f);
		Test->TestEqual(TEXT("Final movement respects 1000 cm per second"), Unit->GetCharacterMovement()->MaxWalkSpeed, 1000.f);
		AAIController* AI = World->SpawnActor<AAIController>();
		AI->Possess(Unit);
		Test->TestTrue(TEXT("Configured AI failure can be forced deterministically"), Combat->RollAIAttackMiss());
		Combat->NotifyAttackResolved(false);
		Test->TestTrue(TEXT("Any miss clears all stacks"), Effects->GetActiveEffectViews().IsEmpty());
		Test->TestEqual(TEXT("Miss resets speed contribution"), Combat->GetAttackSpeedScale(), 1.f);
		Effects->RevokeDefinition(TEXT("Roamer"));
		Test->TestFalse(TEXT("No built-in AI miss chance after revoke"), Combat->RollAIAttackMiss());
		// Exercise actual weapon resolution, not only synthetic buff events.
		UClass* GunnerClass = LoadClass<AGunnerCharacter>(nullptr, TEXT("/Game/MineLearning/Characters/Gunner/Blueprints/BP_Gunner.BP_Gunner_C"));
		if (!Test->TestNotNull(TEXT("Production Gunner class"), GunnerClass)) { return true; }
		AMineableOre* Ore = World->SpawnActor<AMineableOre>(FVector(5000, 0, 500), FRotator::ZeroRotator);
		Ore->FindComponentByClass<UHealthComponent>()->InitializeHealth(100000.f);
		for (int32 Case = 0; Case < 3; ++Case)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const float Distance = Case == 0 ? 2000.f : 200.f;
			AGunnerCharacter* Shooter = World->SpawnActor<AGunnerCharacter>(GunnerClass, Ore->GetActorLocation() - FVector(Distance, 0, 0), FRotator::ZeroRotator, Params);
			Shooter->RestoreLoadedAmmo(20);
			if (Case == 2)
			{
				FCombatModifiers ForcedMiss;
				ForcedMiss.AIMissChance = 1.f;
				Shooter->FindComponentByClass<UCombatComponent>()->SetModifier(TEXT("ForcedAI"), ForcedMiss);
			}
			const float Before = Ore->GetCurrentHealth();
			Test->TestTrue(TEXT("Weapon accepts attempted shot"), Shooter->TryFireAtOre(Ore));
			Test->TestEqual(TEXT("Every shot spends ammo, including out-of-range and AI failure"), Shooter->GetCurrentAmmo(), 19);
			if (Case == 1) { Test->TestTrue(TEXT("Valid shot damages without old random miss"), Ore->GetCurrentHealth() < Before); }
			else { Test->TestEqual(TEXT("Out-of-range or forced AI miss causes no damage"), Ore->GetCurrentHealth(), Before); }
			if (AController* Controller = Shooter->GetController()) { Controller->Destroy(); }
			Shooter->Destroy();
		}
		Ore->Destroy();
		Unit->Destroy();
		AI->Destroy();
		return true;
	}
private:
	FAutomationTestBase* Test;
	ACharacter* Unit = nullptr;
	UCombatComponent* Combat = nullptr;
	UUnitEffectComponent* Effects = nullptr;
	int32 Step = 0;
	float Until = 0.f;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackStacksTest, "MineLearning.Roguelite.AttackStacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAttackStacksTest::RunTest(const FString& Parameters)
{
	FAutomationEditorCommonUtils::CreateNewMap();
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FAttackStackCheck(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
