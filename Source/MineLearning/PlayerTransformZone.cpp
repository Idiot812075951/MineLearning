#include "PlayerTransformZone.h"

#include "Components/BoxComponent.h"
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

	static ConstructorHelpers::FClassFinder<APawn> OreBuddyClass(
		TEXT("/Game/MineLearning/Characters/OreBuddy/Blueprints/BP_OreBuddy07"));
	RobotPawnClass = OreBuddyClass.Class;

	static ConstructorHelpers::FClassFinder<APawn> HumanClass(
		TEXT("/Game/MineLearning/Player/Blueprints/BP_ThirdPersonCharacter"));
	HumanPawnClass = HumanClass.Class;
}

bool APlayerTransformZone::TryTransform(APlayerController* PlayerController)
{
	if (!TrySwapPawn(PlayerController, RobotPawnClass))
	{
		return false;
	}

	UE_LOG(LogPlayerTransformation, Log, TEXT("Player transformed into OreBuddy."));
	return true;
}

bool APlayerTransformZone::TryRestoreHumanForm(APlayerController* PlayerController)
{
	if (!TrySwapPawn(PlayerController, HumanPawnClass))
	{
		return false;
	}

	UE_LOG(LogPlayerTransformation, Log, TEXT("Player restored human form."));
	return true;
}

bool APlayerTransformZone::TrySwapPawn(APlayerController* PlayerController, TSubclassOf<APawn> TargetPawnClass)
{
	APawn* OldPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!OldPawn || !TransformArea->IsOverlappingActor(OldPawn) || !TargetPawnClass || OldPawn->IsA(TargetPawnClass))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FTransform SpawnTransform = OldPawn->GetActorTransform();
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
