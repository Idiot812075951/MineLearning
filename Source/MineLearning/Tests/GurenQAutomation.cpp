#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MineLearning/Interaction/GrabbableComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "MineLearning/Manifestation/Guren/GurenQPresentationComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraDataSetAccessor.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystemInstanceController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrabbableLifecycleTest, "MineLearning.GurenQ.GrabbableLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGrabbableLifecycleTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Initialization);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ACharacter* Grabber = World->SpawnActor<ACharacter>();
	AActor* OtherGrabber = World->SpawnActor<AActor>();
	USkeletalMesh* GurenMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren.SK_Guren"));
	Grabber->GetMesh()->SetSkeletalMeshAsset(GurenMesh);
	Grabber->GetMesh()->SetWorldScale3D(FVector(3.f));
	TestTrue(TEXT("Production grip socket exists"), Grabber->GetMesh()->DoesSocketExist(TEXT("Q_GrabHead")));

	AActor* Target = World->SpawnActor<AActor>();
	UStaticMeshComponent* Root = NewObject<UStaticMeshComponent>(Target);
	Target->SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);
	Root->RegisterComponent();
	Root->SetAbsolute(false, false, true);
	Target->SetActorTransform(FTransform(FRotator(12.f, 70.f, 5.f), FVector(300.f, 50.f, 100.f), FVector(2.f, 3.f, 4.f)));
	UGrabbableComponent* Grip = NewObject<UGrabbableComponent>(Target);
	Grip->SetupAttachment(Root);
	Grip->SetRelativeLocation(FVector(25.f, -15.f, 80.f));
	Grip->GripDiameter = 40.f;
	Grip->RegisterComponent();
	const FTransform Original = Target->GetActorTransform();
	const FVector OriginalAnchor = Grip->GetComponentLocation();
	TestFalse(TEXT("Self-grab rejected"), Grip->CanGrab(Target));
	TestTrue(TEXT("Arbitrary actor with a grip is eligible"), Grip->CanGrab(Grabber));
	TestEqual(TEXT("Authored diameter respects world scale"), Grip->GetGripDiameter(), 160.f);
	TestTrue(TEXT("Reservation succeeds"), Grip->Reserve(Grabber));
	TestFalse(TEXT("Second grabber cannot steal reservation"), Grip->Reserve(OtherGrabber));
	TestFalse(TEXT("Duplicate reserve is rejected"), Grip->Reserve(Grabber));
	TestFalse(TEXT("Missing hand socket fails safely"), Grip->AttachToGrip(Grabber->GetMesh(), TEXT("MissingSocket")));
	TestTrue(TEXT("Generic grip attachment succeeds"), Grip->AttachToGrip(Grabber->GetMesh(), TEXT("Q_GrabHead")));
	TestTrue(TEXT("Authored anchor, not actor pivot, aligns with hand"), Grip->GetComponentLocation().Equals(Grabber->GetMesh()->GetSocketLocation(TEXT("Q_GrabHead")), 0.1f));
	TestTrue(TEXT("Target keeps its scale when grabber is giant"), Target->GetActorScale3D().Equals(Original.GetScale3D()));
	Grabber->GetMesh()->SetWorldRotation(FRotator(25.f, 70.f, 15.f));
	Grip->TickComponent(0.f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Off-center grip remains aligned as the hand rotates"), Grip->GetComponentLocation().Equals(Grabber->GetMesh()->GetSocketLocation(TEXT("Q_GrabHead")), 0.1f));
	TestFalse(TEXT("Held target cannot collide with grabber"), Target->GetActorEnableCollision());
	Grip->Release(false);
	TestTrue(TEXT("Cancellation restores exact target transform"), Target->GetActorTransform().Equals(Original, 0.01f));
	TestTrue(TEXT("Cancellation restores anchor"), Grip->GetComponentLocation().Equals(OriginalAnchor, 0.01f));
	TestTrue(TEXT("Original collision restored"), Target->GetActorEnableCollision());
	TestFalse(TEXT("Original absolute rotation restored"), Root->IsUsingAbsoluteRotation());
	TestTrue(TEXT("Original absolute scale preserved"), Root->IsUsingAbsoluteScale());
	Grip->Release(false);
	TestTrue(TEXT("Repeated cancellation is harmless"), Grip->CanGrab(Grabber));
	Grip->Completion = EGrabCompletion::Restore;
	Grip->Reserve(Grabber);
	Grip->AttachToGrip(Grabber->GetMesh(), TEXT("Q_GrabHead"));
	Grip->Release(true);
	TestTrue(TEXT("Restore completion leaves actor reusable"), Grip->CanGrab(Grabber));
	Grip->bGrabbable = false;
	TestFalse(TEXT("Disabled target rejected"), Grip->CanGrab(Grabber));
	Grip->bGrabbable = true;
	Grip->Completion = EGrabCompletion::Destroy;
	Grip->Reserve(Grabber);
	Grip->Release(true);
	TestFalse(TEXT("Grab completion cannot bypass unified damage and destroy an actor"), Target->IsActorBeingDestroyed());
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGurenPalmRadiationTest, "MineLearning.GurenQ.PalmRadiationLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGurenPalmRadiationTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Initialization);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ACharacter* Character = World->SpawnActor<ACharacter>();
	Character->GetMesh()->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren.SK_Guren")));
	UGurenQSkillComponent* Skill = NewObject<UGurenQSkillComponent>(Character);
	Skill->RegisterComponent();
	UGurenQPresentationComponent* Presentation = NewObject<UGurenQPresentationComponent>(Character);
	Presentation->PalmRadiationSystem = LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/MineLearning/Characters/Guren/VFX/NS_GurenPalmRadiation.NS_GurenPalmRadiation"));
	Presentation->RegisterComponent();
	TestNotNull(TEXT("Production palm effect is configured"), Presentation->PalmRadiationSystem.Get());
	TestTrue(TEXT("Production palm socket exists"), Character->GetMesh()->DoesSocketExist(Presentation->PalmRadiationSocket));
	World->BeginPlay();
	Character->DispatchBeginPlay();
	const int32 CoreIndex = Character->GetMesh()->GetMaterialIndex(Presentation->RadiantMaterialSlot);
	UMaterialInterface* OriginalCore = Character->GetMesh()->GetMaterial(CoreIndex);
	const auto GetEffects = [Character]()
	{
		TArray<UNiagaraComponent*> Effects;
		Character->GetComponents(Effects);
		return Effects;
	};
	Skill->OnGrabContact.Broadcast();
	Skill->OnGrabContact.Broadcast();
	TArray<UNiagaraComponent*> Effects = GetEffects();
	TestEqual(TEXT("Repeated contact creates exactly one effect"), Effects.Num(), 1);
	TArray<UPointLightComponent*> Lights;
	Character->GetComponents(Lights);
	TestEqual(TEXT("Repeated contact creates exactly one local light"), Lights.Num(), 1);
	TestTrue(TEXT("Palm core has an instance for per-cast emission"), Character->GetMesh()->GetMaterial(CoreIndex) != OriginalCore);
	if (Effects.Num() == 1)
	{
		TestEqual(TEXT("Effect attaches to character mesh"), Effects[0]->GetAttachParent(), static_cast<USceneComponent*>(Character->GetMesh()));
		TestEqual(TEXT("Effect uses the authored palm socket"), Effects[0]->GetAttachSocketName(), Presentation->PalmRadiationSocket);
		Character->GetMesh()->SetRelativeScale3D(FVector(3.f));
		TestTrue(TEXT("Effect inherits giant mesh scale once"), Effects[0]->GetComponentScale().Equals(FVector(3.f)));
		// Component scale alone does not prove that Niagara sprite dimensions scale.
		// Read the simulated particles at several sizes, including shrinking again.
		UNiagaraComponent* Effect = Effects[0];
		for (const float Scale : { 1.f, 3.f, 0.5f, 1.f })
		{
			Character->GetMesh()->SetRelativeScale3D(FVector(Scale));
			Presentation->TickComponent(1.f / 30.f, LEVELTICK_All, nullptr);
			if (Lights.Num() == 1)
			{
				TestTrue(TEXT("Local light range scales with the hand"), FMath::IsNearlyEqual(Lights[0]->AttenuationRadius, Presentation->RadiationLightRadius * Scale, 0.01f));
				TestTrue(TEXT("Local light preserves illumination as the source grows"), FMath::IsNearlyEqual(Lights[0]->Intensity, Presentation->RadiationLightIntensity * Scale * Scale, 0.01f));
			}
			Effect->AdvanceSimulation(60, 1.f / 30.f);
			const FNiagaraSystemInstanceControllerPtr Controller = Effect->GetSystemInstanceController();
			if (!TestTrue(TEXT("Palm simulation is active"), Controller.IsValid()))
			{
				break;
			}
			Controller->WaitForConcurrentTickAndFinalize();
			FNiagaraSystemInstance* Instance = Controller->GetSystemInstance_Unsafe();
			if (!TestNotNull(TEXT("Palm simulation instance"), Instance))
			{
				break;
			}
			for (const FNiagaraEmitterInstanceRef& Emitter : Instance->GetEmitters())
			{
				const FName Name = Emitter->GetEmitterHandle().GetName();
				// Short arcs intentionally have gaps; sample over one spawn interval.
				for (int32 Frame = 0; Emitter->GetNumParticles() == 0 && Frame < 15; ++Frame)
				{
					Effect->AdvanceSimulation(1, 1.f / 30.f);
					Controller->WaitForConcurrentTickAndFinalize();
				}
				const FVector2f Range = Name == TEXT("CorePulse") ? FVector2f(76.f, 84.f)
					: Name == TEXT("HeatDistortion") ? FVector2f(116.f, 130.f)
					: Name == TEXT("HotSparks") ? FVector2f(1.2f, 2.5f)
					: Name == TEXT("Arc_Micro") ? FVector2f(66.f, 76.f) : FVector2f(112.f, 124.f);
				const FNiagaraDataSet& Data = Emitter->GetParticleData();
				const FNiagaraDataSetAccessorFloat<FLinearColor> ParticleColors(Data, TEXT("Color"));
				if (ParticleColors.IsValid() && Emitter->GetNumParticles() > 0)
				{
					TestTrue(TEXT("Live particles have visible color alpha"), ParticleColors.GetReader(Data).GetSafe(0, FLinearColor::Transparent).A > 0.f);
				}
				const FNiagaraDataSetAccessorFloat<FVector4f> IntensityParameters(Data, TEXT("DynamicMaterialParameter"));
				if (TestTrue(TEXT("Renderer has unified intensity parameters"), IntensityParameters.IsValid()) && Emitter->GetNumParticles() > 0)
				{
					const FVector4f ParametersValue = IntensityParameters.GetReader(Data).GetSafe(0, FVector4f(0.f));
					TestTrue(TEXT("Active core and heat intensities reach the particles"), ParametersValue.X > 0.f && ParametersValue.Y > 0.f);
				}
				// Compiled particle datasets strip the "Particles." namespace.
				const FNiagaraDataSetAccessorFloat<FVector2f> Sizes(Data, TEXT("SpriteSize"));
				if (!TestTrue(TEXT("Renderer sprite size exists in particle data"), Sizes.IsValid()))
				{
					continue;
				}
				TestTrue(FString::Printf(TEXT("%s emits within its spawn interval"), *Name.ToString()), Emitter->GetNumParticles() > 0);
				const FNiagaraDataSetReaderFloat<FVector2f> Reader = Sizes.GetReader(Data);
				for (int32 Index = 0; Index < Emitter->GetNumParticles(); ++Index)
				{
					const FVector2f Size = Reader.GetSafe(Index, FVector2f::ZeroVector) / Scale;
					TestTrue(FString::Printf(TEXT("%s sprite dimensions track mesh scale %.2f without accumulating"), *Name.ToString(), Scale),
						Size.X >= Range.X - 0.1f && Size.X <= Range.Y + 0.1f
						&& Size.Y >= Range.X - 0.1f && Size.Y <= Range.Y + 0.1f);
				}
			}
		}
	}
	Skill->OnStageChanged.Broadcast(EGurenQStage::Release, nullptr);
	TestEqual(TEXT("Normal release retains only the brief heat tail"), GetEffects().Num(), 1);
	World->Tick(LEVELTICK_All, 0.2f);
	TestEqual(TEXT("Heat tail expires and removes all particles"), GetEffects().Num(), 0);
	Character->GetComponents(Lights);
	TestEqual(TEXT("Heat tail releases the local light"), Lights.Num(), 0);
	TestEqual(TEXT("Release restores the exact palm material"), Character->GetMesh()->GetMaterial(CoreIndex), OriginalCore);
	Skill->OnGrabContact.Broadcast();
	Skill->OnStageChanged.Broadcast(EGurenQStage::Idle, nullptr);
	TestEqual(TEXT("Cancelled execution removes all particles"), GetEffects().Num(), 0);
	TestEqual(TEXT("Cancellation restores the exact palm material"), Character->GetMesh()->GetMaterial(CoreIndex), OriginalCore);
	Presentation->PalmRadiationSocket = TEXT("MissingPalmSocket");
	Skill->OnGrabContact.Broadcast();
	TestEqual(TEXT("Missing socket does not spawn at world origin"), GetEffects().Num(), 0);
	Presentation->PalmRadiationSocket = TEXT("socket_palm_fx");
	Skill->OnGrabContact.Broadcast();
	Effects = GetEffects();
	TWeakObjectPtr<UNiagaraComponent> LastEffect = Effects.IsEmpty() ? nullptr : Effects[0];
	Presentation->DestroyComponent();
	TestTrue(TEXT("Presentation teardown destroys its effect"), !LastEffect.IsValid() || !LastEffect->IsRegistered());
	TestEqual(TEXT("Presentation teardown leaves no owned effect"), GetEffects().Num(), 0);
	// Route actor EndPlay while the test world's timer manager is still alive.
	Character->Destroy();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
