#include "PlayerTransformZone.h"
#include "TransformationGuard.h"
#include "Demo/DemoRunComponent.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogPlayerTransformation, Log, All);

APlayerTransformZone::APlayerTransformZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TransformArea = CreateDefaultSubobject<UBoxComponent>(TEXT("TransformArea"));
	SetRootComponent(TransformArea);
	TransformArea->SetBoxExtent(FVector(250.0f, 250.0f, 150.0f));
	TransformArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TransformArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	TransformArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TransformArea->SetGenerateOverlapEvents(true);
	TransformArea->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FClassFinder<APawn> GunnerClass(
		TEXT("/Game/MineLearning/Characters/Gunner/Blueprints/BP_Gunner"));
	RobotPawnClass = GunnerClass.Class;

	static ConstructorHelpers::FClassFinder<APawn> OreBuddyClass(
		TEXT("/Game/MineLearning/Characters/OreBuddy/Blueprints/BP_OreBuddy07"));
	OreBuddyPawnClass = OreBuddyClass.Class;

	static ConstructorHelpers::FClassFinder<APawn> HumanClass(
		TEXT("/Game/MineLearning/Player/Blueprints/BP_ThirdPersonCharacter"));
	HumanPawnClass = HumanClass.Class;
	GurenPawnClass = TSoftClassPtr<APawn>(FSoftObjectPath(
		TEXT("/Game/MineLearning/Characters/Guren/Blueprints/BP_GurenPlayer.BP_GurenPlayer_C")));
	CarrierPawnClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/MineLearning/Mining/Logistics/Blueprints/BP_Hauler.BP_Hauler_C")));
}

bool APlayerTransformZone::TryTransform(APlayerController* PlayerController)
{
	return TrySelectForm(PlayerController, EPlayerTransformationForm::Gunner);
}

bool APlayerTransformZone::TryRestoreHumanForm(APlayerController* PlayerController)
{
	return TrySelectForm(PlayerController, EPlayerTransformationForm::Human);
}

bool APlayerTransformZone::TrySelectForm(
	APlayerController* PlayerController,
	const EPlayerTransformationForm Form)
{
	const UDemoRunComponent* Run = PlayerController ? PlayerController->FindComponentByClass<UDemoRunComponent>() : nullptr;
	if (Run && Run->IsEnabled() && (!Run->IsFormUnlocked(Form) || Run->GetPhase() == EDemoPhase::Briefing))
	{
		return false;
	}
	TSubclassOf<APawn> TargetPawnClass;
	switch (Form)
	{
	case EPlayerTransformationForm::Human:
		TargetPawnClass = HumanPawnClass;
		break;
	case EPlayerTransformationForm::OreBuddy:
		TargetPawnClass = OreBuddyPawnClass;
		break;
	case EPlayerTransformationForm::Gunner:
		TargetPawnClass = RobotPawnClass;
		break;
	case EPlayerTransformationForm::Guren:
		TargetPawnClass = GurenPawnClass.LoadSynchronous();
		break;
	case EPlayerTransformationForm::Carrier:
		TargetPawnClass = CarrierPawnClass.LoadSynchronous();
		break;
	default:
		return false;
	}

	if (!TrySwapPawn(PlayerController, TargetPawnClass))
	{
		return false;
	}

	UE_LOG(LogPlayerTransformation, Log, TEXT("Player selected transformation form %s."),
		*UEnum::GetValueAsString(Form));
	return true;
}

bool APlayerTransformZone::TryTransformOverlappingPlayer(APlayerController* PlayerController)
{
	APlayerTransformZone* TransformZone = FindOverlappingZone(PlayerController);
	return TransformZone && TransformZone->TryTransform(PlayerController);
}

bool APlayerTransformZone::TryRestoreOverlappingPlayer(APlayerController* PlayerController)
{
	APlayerTransformZone* TransformZone = FindOverlappingZone(PlayerController);
	return TransformZone && TransformZone->TryRestoreHumanForm(PlayerController);
}

bool APlayerTransformZone::TrySelectOverlappingForm(
	APlayerController* PlayerController,
	const EPlayerTransformationForm Form)
{
	APlayerTransformZone* TransformZone = FindOverlappingZone(PlayerController);
	return TransformZone && TransformZone->TrySelectForm(PlayerController, Form);
}

bool APlayerTransformZone::IsPlayerOverlappingTransformZone(APlayerController* PlayerController)
{
	return FindOverlappingZone(PlayerController) != nullptr;
}

APlayerTransformZone* APlayerTransformZone::FindOverlappingZone(APlayerController* PlayerController)
{
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return nullptr;
	}

	TArray<AActor*> OverlappingZones;
	Pawn->GetOverlappingActors(OverlappingZones, StaticClass());
	for (AActor* Actor : OverlappingZones)
	{
		if (APlayerTransformZone* TransformZone = Cast<APlayerTransformZone>(Actor))
		{
			return TransformZone;
		}
	}

	return nullptr;
}

bool APlayerTransformZone::TrySwapPawn(APlayerController* PlayerController, TSubclassOf<APawn> TargetPawnClass)
{
	APawn* OldPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UDemoRunComponent* Run = PlayerController ? PlayerController->FindComponentByClass<UDemoRunComponent>() : nullptr;
	const bool bFieldTransform = Run && Run->IsEnabled() && Run->GetPhase() != EDemoPhase::Briefing;
	if (!OldPawn || (!bFieldTransform && !TransformArea->IsOverlappingActor(OldPawn)) || !TargetPawnClass)
	{
		return false;
	}
	if (OldPawn->IsA(TargetPawnClass))
	{
		return true;
	}
	if (const ITransformationGuard* Guard = Cast<ITransformationGuard>(OldPawn); Guard && !Guard->CanTransform())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const APawn* TargetPawnCDO = TargetPawnClass->GetDefaultObject<APawn>();
	FVector SpawnLocation = OldPawn->GetActorLocation();
	const UCapsuleComponent* OldCapsule = OldPawn->FindComponentByClass<UCapsuleComponent>();
	const UCapsuleComponent* TargetCapsule = TargetPawnCDO
		? TargetPawnCDO->FindComponentByClass<UCapsuleComponent>()
		: nullptr;
	if (OldCapsule && TargetCapsule)
	{
		// Preserve foot height when forms use different CDO scale/capsule sizes.
		SpawnLocation.Z += TargetCapsule->GetScaledCapsuleHalfHeight()
			- OldCapsule->GetScaledCapsuleHalfHeight();
	}
	// The deferred spawn already applies the target class default root scale. Passing
	// that scale again would multiply it (for example, BP_Gunner 2x would become 4x).
	const FTransform SpawnTransform(OldPawn->GetActorRotation(), SpawnLocation, FVector::OneVector);
	const FRotator ControlRotation = PlayerController->GetControlRotation();
	APawn* NewPawn = World->SpawnActorDeferred<APawn>(
		TargetPawnClass,
		SpawnTransform,
		nullptr,
		OldPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!NewPawn)
	{
		UE_LOG(LogPlayerTransformation, Error, TEXT("Could not spawn transformation pawn class '%s'."), *GetNameSafe(TargetPawnClass.Get()));
		return false;
	}

	NewPawn->AutoPossessAI = EAutoPossessAI::Disabled;
	UGameplayStatics::FinishSpawningActor(NewPawn, SpawnTransform);
	PlayerController->Possess(NewPawn);

	if (PlayerController->GetPawn() != NewPawn)
	{
		NewPawn->Destroy();
		PlayerController->Possess(OldPawn);
		return false;
	}

	if (UResourceCarryComponent* CarryComponent = OldPawn->FindComponentByClass<UResourceCarryComponent>();
		CarryComponent && !CarryComponent->IsEmpty())
	{
		const FVector DropOrigin = OldPawn->GetActorLocation()
			+ OldPawn->GetActorForwardVector() * 90.0f;
		if (CarryComponent->DropAllItems(DropOrigin) <= 0)
		{
			PlayerController->Possess(OldPawn);
			NewPawn->Destroy();
			return false;
		}
	}

	PlayerController->SetControlRotation(ControlRotation);
	OldPawn->Destroy();
	return true;
}
