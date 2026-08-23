#include "HaulerCharacter.h"

#include "HaulerAIController.h"
#include "CarrierAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/ItemTypes.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AHaulerCharacter::AHaulerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	GetCapsuleComponent()->InitCapsuleSize(36.0f, 64.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 320.0f;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CarrierMesh(
		TEXT("/Game/MineLearning/Characters/Carrier/SK_CarrierRobot.SK_CarrierRobot"));
	if (CarrierMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(CarrierMesh.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -64.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> CarrierAnim(
		TEXT("/Game/MineLearning/Characters/Carrier/ABP_CarrierRobot"));
	if (CarrierAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(CarrierAnim.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> PickupAnimFinder(
		TEXT("/Game/MineLearning/Characters/Carrier/CarrierRobotRIG_CarrierRobot_AN_Carrier_PickUp.CarrierRobotRIG_CarrierRobot_AN_Carrier_PickUp"));
	PickupAnimation = PickupAnimFinder.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DropOffAnimFinder(
		TEXT("/Game/MineLearning/Characters/Carrier/CarrierRobotRIG_CarrierRobot_AN_Carrier_DropOff.CarrierRobotRIG_CarrierRobot_AN_Carrier_DropOff"));
	DropOffAnimation = DropOffAnimFinder.Object;

	ResourceCarryComponent = CreateDefaultSubobject<UResourceCarryComponent>(TEXT("ResourceCarryComponent"));
	// The Carrier owns the processor -> warehouse leg. OreBuddy remains responsible
	// for ore extraction/transport, so this policy intentionally accepts currency only.
	ResourceCarryComponent->ConfigureAcceptance(1, false, { EItemCategory::Currency });

	CarriedItemVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarriedItemVisual"));
	CarriedItemVisual->SetupAttachment(GetMesh(), TEXT("S_Cargo"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CargoBoxFinder(
		TEXT("/Game/MineLearning/Characters/Carrier/SM_Carrier_CargoBox.SM_Carrier_CargoBox"));
	CarriedItemVisual->SetStaticMesh(CargoBoxFinder.Object);
	CarriedItemVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarriedItemVisual->SetGenerateOverlapEvents(false);
	CarriedItemVisual->SetCanEverAffectNavigation(false);
	CarriedItemVisual->SetVisibility(false, true);

	CargoContentVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoContentVisual"));
	CargoContentVisual->SetupAttachment(CarriedItemVisual);
	CargoContentVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
	CargoContentVisual->SetRelativeScale3D(FVector(MineLearningItemVisual::GoldCoinScale));
	CargoContentVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CargoContentVisual->SetGenerateOverlapEvents(false);
	CargoContentVisual->SetCanEverAffectNavigation(false);
	CargoContentVisual->SetVisibility(false, true);

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AHaulerAIController::StaticClass();
}

void AHaulerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Logistics pawns share the same narrow machine aisle. Pawn blocking can
	// deadlock OreBuddy and the carrier nose-to-nose.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);
	CargoContentVisual->SetRelativeScale3D(FVector(MineLearningItemVisual::GoldCoinScale));
}

void AHaulerCharacter::ShowCarriedItem(UStaticMesh* ItemMesh)
{
	CargoContentVisual->SetStaticMesh(ItemMesh);
	CargoContentVisual->SetRelativeScale3D(FVector(MineLearningItemVisual::GoldCoinScale));
	CarriedItemVisual->SetVisibility(true, true);
	CargoContentVisual->SetVisibility(IsValid(ItemMesh), true);
}

void AHaulerCharacter::HideCarriedItem()
{
	CarriedItemVisual->SetVisibility(false, true);
	CargoContentVisual->SetVisibility(false, true);
	CargoContentVisual->SetStaticMesh(nullptr);
}

void AHaulerCharacter::PlayPickupAnimation()
{
	PlayInteractionAnimation(PickupAnimation, true);
}

void AHaulerCharacter::PlayDropOffAnimation()
{
	PlayInteractionAnimation(DropOffAnimation, false);
}

void AHaulerCharacter::PlayInteractionAnimation(UAnimSequence* Sequence, bool bPickup)
{
	GetWorldTimerManager().ClearTimer(InteractionNotifyFallbackHandle);
	GetWorldTimerManager().ClearTimer(InteractionFinishedHandle);

	const float Duration = IsValid(Sequence) ? Sequence->GetPlayLength() : 0.1f;
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); IsValid(Sequence) && AnimInstance)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(
			Sequence, TEXT("DefaultSlot"), 0.08f, 0.10f, 1.0f, 1, -1.0f, 0.0f);
	}

	if (bPickup)
	{
		GetWorldTimerManager().SetTimer(
			InteractionNotifyFallbackHandle, this, &AHaulerCharacter::HandlePickupNotify,
			FMath::Max(0.05f, Duration * 0.88f), false);
		GetWorldTimerManager().SetTimer(
			InteractionFinishedHandle, this, &AHaulerCharacter::HandlePickupAnimationFinished,
			Duration + 0.12f, false);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			InteractionNotifyFallbackHandle, this, &AHaulerCharacter::HandleDropOffNotify,
			FMath::Max(0.05f, Duration * 0.88f), false);
		GetWorldTimerManager().SetTimer(
			InteractionFinishedHandle, this, &AHaulerCharacter::HandleDropOffAnimationFinished,
			Duration + 0.12f, false);
	}
}

void AHaulerCharacter::HandlePickupNotify()
{
	GetWorldTimerManager().ClearTimer(InteractionNotifyFallbackHandle);
	if (AHaulerAIController* HaulerController = Cast<AHaulerAIController>(GetController()))
	{
		HaulerController->HandlePickupAnimationNotify();
	}
}

void AHaulerCharacter::HandleDropOffNotify()
{
	GetWorldTimerManager().ClearTimer(InteractionNotifyFallbackHandle);
	if (AHaulerAIController* HaulerController = Cast<AHaulerAIController>(GetController()))
	{
		HaulerController->HandleDropOffAnimationNotify();
	}
}

void AHaulerCharacter::HandlePickupAnimationFinished()
{
	if (AHaulerAIController* HaulerController = Cast<AHaulerAIController>(GetController()))
	{
		HaulerController->HandlePickupAnimationFinished();
	}
}

void AHaulerCharacter::HandleDropOffAnimationFinished()
{
	if (AHaulerAIController* HaulerController = Cast<AHaulerAIController>(GetController()))
	{
		HaulerController->HandleDropOffAnimationFinished();
	}
}

bool AHaulerCharacter::HasVisibleCargo() const
{
	return CarriedItemVisual && CarriedItemVisual->IsVisible();
}
