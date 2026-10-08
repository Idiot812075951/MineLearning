#include "HaulerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/HealthComponent.h"


#include "HaulerAIController.h"
#include "CarrierAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Navigation/NavigationStandards.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AHaulerCharacter::AHaulerCharacter()
{
	UHealthComponent* Health = CreateDefaultSubobject<UHealthComponent>(TEXT("CombatHealth"));
	Health->Faction = ECombatFaction::Player;
	UCombatComponent* Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->Config = TSoftObjectPtr<UCombatConfig>(FSoftObjectPath(TEXT("/Game/MineLearning/Combat/DA_CarrierCombat.DA_CarrierCombat")));

	PrimaryActorTick.bCanEverTick = false;
	PlayerCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("PlayerCameraBoom"));
	PlayerCameraBoom->SetupAttachment(GetRootComponent());
	PlayerCameraBoom->TargetArmLength = 450.f;
	PlayerCameraBoom->bUsePawnControlRotation = true;
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(PlayerCameraBoom, USpringArmComponent::SocketName);
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> Mapping(TEXT("/Game/MineLearning/Input/IMC_Default.IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Move(TEXT("/Game/MineLearning/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Look(TEXT("/Game/MineLearning/Input/Actions/IA_Look.IA_Look"));
	PlayerMapping = Mapping.Object;
	PlayerMoveAction = Move.Object;
	PlayerLookAction = Look.Object;
	GetCapsuleComponent()->InitCapsuleSize(36.0f, 64.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 360.0f;
	GetCharacterMovement()->MaxStepHeight = MineLearningNavigation::CharacterStepHeight;

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
	ResourceCarryComponent->bAcceptsLogisticsOrders = true;
	// The Carrier owns player-scheduled warehouse routes in both directions:
	// ore goes to the processor, ingots to the seller, and outputs return to storage.
	ResourceCarryComponent->ConfigureAcceptance(
		4,
		false,
		{ EItemCategory::Ore, EItemCategory::Currency, EItemCategory::ProcessedMaterial });

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
}

void AHaulerCharacter::ShowCarriedItem(UStaticMesh* ItemMesh)
{
	// The carry component owns the physical item pile. This component is only
	// the authored tray, avoiding two independent visual-count systems.
	CargoContentVisual->SetStaticMesh(nullptr);
	CargoContentVisual->SetVisibility(false, true);
	CarriedItemVisual->SetVisibility(IsValid(ItemMesh), true);
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

	// This action is both an attack and a skill: combine bonuses, not multipliers.
	const UCombatComponent* Combat = FindComponentByClass<UCombatComponent>();
	const FCombatModifiers Bonuses = Combat ? Combat->GetModifiers() : FCombatModifiers();
	const float BaseDuration = IsValid(Sequence) ? Sequence->GetPlayLength() : 0.2f;
	const UCombatConfig* Config = Combat ? Combat->GetConfig() : nullptr;
	const float MaxRate = Config ? Config->MaxAttacksPerSecond : 5.f;
	const float Speed = FMath::Min(FMath::Max(0.1f, 1.f + Bonuses.AttackSpeed + Bonuses.CastSpeed), BaseDuration * MaxRate);
	const float Duration = BaseDuration / Speed;
	if (IsPlayerControlled())
	{
		NextPlayerTransferTime = GetWorld()->GetTimeSeconds() + Duration + 0.12f / Speed;
	}
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); IsValid(Sequence) && AnimInstance)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(
			Sequence, TEXT("DefaultSlot"), 0.08f / Speed, 0.10f / Speed, Speed, 1, -1.0f, 0.0f);
	}

	if (bPickup)
	{
		GetWorldTimerManager().SetTimer(
			InteractionNotifyFallbackHandle, this, &AHaulerCharacter::HandlePickupNotify,
			FMath::Max(0.05f, Duration * 0.88f), false);
		GetWorldTimerManager().SetTimer(
			InteractionFinishedHandle, this, &AHaulerCharacter::HandlePickupAnimationFinished,
			Duration + 0.12f / Speed, false);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			InteractionNotifyFallbackHandle, this, &AHaulerCharacter::HandleDropOffNotify,
			FMath::Max(0.05f, Duration * 0.88f), false);
		GetWorldTimerManager().SetTimer(
			InteractionFinishedHandle, this, &AHaulerCharacter::HandleDropOffAnimationFinished,
			Duration + 0.12f / Speed, false);
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

float AHaulerCharacter::GetInteractionRange() const
{
	const UCombatComponent* Combat = FindComponentByClass<UCombatComponent>();
	return Combat ? Combat->GetAttackRange() : 260.f;
}
