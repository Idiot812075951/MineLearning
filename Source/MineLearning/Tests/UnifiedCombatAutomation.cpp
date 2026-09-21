#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/CombatDamageSubsystem.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "MineLearning/Interaction/GrabbableComponent.h"
#include "MineLearning/Manifestation/Guren/GurenQSkillComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnifiedCombatTest, "MineLearning.Combat.DamageAndExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnifiedCombatTest::RunTest(const FString& Parameters)
{
	const UCombatComponent* MiningDefaults = GetDefault<AMiningCompanionCharacter>()->FindComponentByClass<UCombatComponent>();
	const UCombatConfig* MiningConfig = MiningDefaults ? MiningDefaults->GetConfig() : nullptr;
	if (TestNotNull(TEXT("Native mining defaults resolve the shipped Combat DataAsset"), MiningConfig))
	{
		const FSkillDamageSpec* MiningSkill = MiningConfig->FindSkill(TEXT("Primary"));
		if (TestNotNull(TEXT("Mining configuration supplies the primary skill"), MiningSkill))
		{
			TestTrue(TEXT("Configured mining damage is positive"), MiningSkill->Evaluate(MiningConfig->Attributes) > 0.f);
		}
	}
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Initialization);
	if (!TestNotNull(TEXT("World"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APlayerController* Player = World->SpawnActor<APlayerController>();
	Player->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	ACharacter* Source = World->SpawnActor<ACharacter>();
	Player->Possess(Source);
	Source->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Source->GetMesh()->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren.SK_Guren")));
	UHealthComponent* SourceHealth = NewObject<UHealthComponent>(Source);
	SourceHealth->Faction = ECombatFaction::Player;
	SourceHealth->RegisterComponent();
	UCombatComponent* Combat = NewObject<UCombatComponent>(Source);
	UCombatConfig* Config = NewObject<UCombatConfig>(Combat);
	Config->Attributes = {20.f, 10.f, 30.f};
	FSkillDamageSpec Spec;
	Spec.SkillId = TEXT("Primary");
	Spec.BaseDamage = 40.f;
	Spec.StrengthScale = 0.8f;
	Spec.AgilityScale = 0.3f;
	Spec.IntelligenceScale = 0.6f;
	Config->Skills.Add(Spec);
	Spec.SkillId = TEXT("QGrab");
	Config->Skills.Add(Spec);
	Spec.SkillId = TEXT("QRadiation");
	Config->Skills.Add(Spec);
	Combat->Config = Config;
	Combat->RegisterComponent();
	UGurenQSkillComponent* Q = NewObject<UGurenQSkillComponent>(Source);
	Q->RegisterComponent();
	AActor* Target = World->SpawnActor<AActor>();
	UStaticMeshComponent* Root = NewObject<UStaticMeshComponent>(Target);
	Root->SetMobility(EComponentMobility::Movable);
	Target->SetRootComponent(Root);
	Root->RegisterComponent();
	Target->SetActorLocation(FVector(165.f, 53.f, 0.f));
	UHealthComponent* Health = NewObject<UHealthComponent>(Target);
	Health->RegisterComponent();
	Health->InitializeHealth(5000.f);
	UGrabbableComponent* Grab = NewObject<UGrabbableComponent>(Target);
	Grab->SetupAttachment(Root);
	Grab->RegisterComponent();
	UCombatDamageSubsystem* Damage = World->GetSubsystem<UCombatDamageSubsystem>();
	FCombatDamageRequest Request;
	Request.Source = Source;
	Request.Target = Target;
	Request.SkillId = TEXT("Primary");
	Request.Multiplier = 2.f;
	TestEqual(TEXT("Fixed plus all three attributes, then mechanic multiplier"), Damage->ApplyDamage(Request).AppliedDamage, 154.f);
	TestEqual(TEXT("UI formula reads the same source"), Combat->GetPanelData().Skills[0].Damage, 77.f);
	Health->Faction = ECombatFaction::Player;
	TestFalse(TEXT("Friendly damage rejected"), Damage->ApplyDamage(Request).bAccepted);
	Health->Faction = ECombatFaction::Hostile;
	Request.Multiplier = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("Invalid multiplier rejected"), Damage->ApplyDamage(Request).bAccepted);
	Request.Multiplier = 1.f;
	Request.SkillId = TEXT("Missing");
	TestFalse(TEXT("Missing skill cannot silently deal damage"), Damage->ApplyDamage(Request).bAccepted);
	TestFalse(TEXT("Negative GM damage cannot heal"), Damage->ApplyDebugDamage(Target, -1.f).bAccepted);
	TestEqual(TEXT("Fixed threshold dominates small health pools"), Q->GetExecuteThreshold(1800.f), 1000.f);
	TestEqual(TEXT("Percent threshold dominates large health pools"), Q->GetExecuteThreshold(5000.f), 2500.f);
	Damage->ApplyDebugDamage(Target, Health->GetHealth() - 2501.f);
	Q->RefreshTargets();
	TestTrue(TEXT("Above-threshold target has no Q indicator"), Q->GetCandidates().IsEmpty());
	Damage->ApplyDebugDamage(Target, 1.f);
	Q->RefreshTargets();
	TestEqual(TEXT("Exact percent boundary has a Q indicator"), Q->GetCandidates().Num(), 1);
	Q->TryCast();
	TestTrue(TEXT("Eligible target starts Q"), Q->IsQActive());
	// Move the transient grip into actual contact; no keyboard, animation delays or private state writes.
	Target->SetActorLocation(Q->GetGripLocation());
	Q->HandleAnimationEvent(TEXT("GrabContact"));
	TestEqual(TEXT("Contact really attaches target"), Target->GetAttachParentActor(), static_cast<AActor*>(Source));
	Health->Heal(5000.f);
	TestEqual(TEXT("Target can heal above execution line after contact"), Health->GetHealth(), 5000.f);
	Q->HandleAnimationEvent(TEXT("StartDissolve"));
	Q->HandleAnimationEvent(TEXT("DissolveFinish"));
	TestTrue(TEXT("Confirmed grab remains executable after healing"), Target->IsActorBeingDestroyed());
	TestEqual(TEXT("Execution settles through health"), Health->GetHealth(), 0.f);
	Q->HandleAnimationEvent(TEXT("SkillEnd"));
	TestFalse(TEXT("Q returns idle"), Q->IsQActive());
	Source->Destroy();
	Player->Destroy();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
