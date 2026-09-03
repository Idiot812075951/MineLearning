#include "MiningCompanionCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "MineLearning/Mining/ItemLogisticsLibrary.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Mining/ItemReceiver.h"
#include "MineLearning/Mining/MiningToolComponent.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/OreProcessorMachine.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/ResourceDepot.h"
#include "MineLearning/AI/MiningCompanionAIController.h"
#include "MineLearning/Navigation/NavigationStandards.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "OreBuddyAnimInstance.h"
#include "UObject/ConstructorHelpers.h"

AMiningCompanionCharacter::AMiningCompanionCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	// Match the human pawn so player-controlled OreBuddy does not snag on the
	// same functional-pad lips immediately after transforming.
	GetCharacterMovement()->MaxStepHeight = MineLearningNavigation::CharacterStepHeight;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("PlayerCameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerFollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextFinder(
		TEXT("/Game/MineLearning/Input/IMC_Default.IMC_Default"));
	PlayerMappingContext = MappingContextFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(
		TEXT("/Game/MineLearning/Input/Actions/IA_Move.IA_Move"));
	MoveAction = MoveActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(
		TEXT("/Game/MineLearning/Input/Actions/IA_Look.IA_Look"));
	LookAction = LookActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> MiningSkillActionFinder(
		TEXT("/Game/MineLearning/Input/Actions/IA_RobotSkill1.IA_RobotSkill1"));
	MiningSkillAction = MiningSkillActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> PickupSkillActionFinder(
		TEXT("/Game/MineLearning/Input/Actions/IA_RobotPickup.IA_RobotPickup"));
	PickupSkillAction = PickupSkillActionFinder.Object;

	MiningToolComponent = CreateDefaultSubobject<UMiningToolComponent>(TEXT("MiningToolComponent"));
	MiningToolComponent->SetMiningHitSocketName(TEXT("S_DrillTip"));
	ResourceCarryComponent = CreateDefaultSubobject<UResourceCarryComponent>(TEXT("ResourceCarryComponent"));
	ResourceCarryComponent->ConfigureAcceptance(4, false, {EItemCategory::Ore});

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AMiningCompanionCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PlayerInteractionState == EPlayerInteractionState::AligningCollect
		|| PlayerInteractionState == EPlayerInteractionState::AligningDeposit)
	{
		UpdatePlayerInteractionAlignment(DeltaSeconds);
	}
}

void AMiningCompanionCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayerInteractionTimerHandle);
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (PlayerMappingContext)
			{
				Subsystem->AddMappingContext(PlayerMappingContext, 0);
			}
		}

		StartPlayerInteractionMonitoring(PlayerController);
		OnControlModeChanged.Broadcast(true);
	}
	else
	{
		if (PlayerInteractionState != EPlayerInteractionState::None)
		{
			FinishPlayerInteraction();
		}
		const bool bAvailabilityChanged = bMiningSkillAvailable || bPickupSkillAvailable;
		bMiningSkillAvailable = false;
		bPickupSkillAvailable = false;
		if (bAvailabilityChanged)
		{
			OnSkillAvailabilityChanged.Broadcast(false, false);
		}
		OnControlModeChanged.Broadcast(false);
	}
}

void AMiningCompanionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMiningCompanionCharacter::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMiningCompanionCharacter::Look);
		}

		if (MiningSkillAction)
		{
			EnhancedInputComponent->BindAction(
				MiningSkillAction,
				ETriggerEvent::Started,
				this,
				&AMiningCompanionCharacter::TryUseMiningSkill);
		}

		if (PickupSkillAction)
		{
			EnhancedInputComponent->BindAction(
				PickupSkillAction,
				ETriggerEvent::Started,
				this,
				&AMiningCompanionCharacter::TryUsePickupSkill);
		}
	}

	StartPlayerInteractionMonitoring(Cast<APlayerController>(Controller));
}

void AMiningCompanionCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller || IsPlayerActionLocked())
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementVector.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementVector.X);
}

void AMiningCompanionCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AMiningCompanionCharacter::TryUseMiningSkill()
{
	if (IsPlayerActionLocked())
	{
		return;
	}

	AMineableOre* TargetOre = FindMineableOreInRange();
	if (!TargetOre)
	{
		ShowNoMineableOreMessage();
		return;
	}

	if (!MiningToolComponent || MiningToolComponent->IsMining())
	{
		return;
	}

	FVector ClosestPoint;
	GetSquaredDistanceToOre(TargetOre, &ClosestPoint);
	FVector TargetDirection = ClosestPoint - GetActorLocation();
	TargetDirection.Z = 0.0f;
	if (!TargetDirection.IsNearlyZero())
	{
		SetActorRotation(TargetDirection.Rotation());
	}

	MiningToolComponent->StartMiningTarget(TargetOre);
}

void AMiningCompanionCharacter::TryUsePickupSkill()
{
	if (!ResourceCarryComponent || IsPlayerActionLocked())
	{
		return;
	}

	if (ResourceCarryComponent->IsFull())
	{
		ShowCarryFullMessage();
		return;
	}

	AItemPickup* TargetPickup = FindPickupInRange();
	if (!TargetPickup)
	{
		ShowNoPickupMessage();
		return;
	}

	StartPlayerCollectAction(TargetPickup);
}

AMineableOre* AMiningCompanionCharacter::FindMineableOreInRange() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AMineableOre* NearestOre = nullptr;
	float NearestDistanceSq = FMath::Square(PlayerMiningInteractRadius);
	for (TActorIterator<AMineableOre> It(World); It; ++It)
	{
		AMineableOre* Ore = *It;
		if (!IsValid(Ore) || Ore->IsActorBeingDestroyed() || Ore->IsDestroyed())
		{
			continue;
		}

		const float DistanceSq = GetSquaredDistanceToOre(Ore);
		if (DistanceSq <= NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			NearestOre = Ore;
		}
	}

	return NearestOre;
}

AItemPickup* AMiningCompanionCharacter::FindPickupInRange()
{
	UWorld* World = GetWorld();
	if (!World || !ResourceCarryComponent || ResourceCarryComponent->IsFull())
	{
		return nullptr;
	}

	AItemPickup* NearestPickup = nullptr;
	float NearestDistanceSq = FMath::Square(PlayerPickupInteractRadius);
	for (TActorIterator<AItemPickup> It(World); It; ++It)
	{
		AItemPickup* Pickup = *It;
		if (!IsValid(Pickup) || Pickup->IsActorBeingDestroyed() || Pickup->GetAmount() <= 0
			|| !Pickup->IsAvailableFor(this)
			|| !ResourceCarryComponent->CanAcceptItem(Pickup->GetItemStack()))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), Pickup->GetActorLocation());
		if (DistanceSq <= NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			NearestPickup = Pickup;
		}
	}

	return NearestPickup;
}

float AMiningCompanionCharacter::GetSquaredDistanceToOre(
	const AMineableOre* Ore,
	FVector* OutClosestPoint) const
{
	const FVector SearchLocation = GetActorLocation();
	FVector ClosestPoint = Ore ? Ore->GetActorLocation() : SearchLocation;
	float DistanceSq = FVector::DistSquared(SearchLocation, ClosestPoint);

	if (Ore)
	{
		if (UStaticMeshComponent* OreMesh = Ore->GetOreMesh())
		{
			const float SurfaceDistance = OreMesh->GetClosestPointOnCollision(SearchLocation, ClosestPoint);
			if (SurfaceDistance >= 0.0f)
			{
				DistanceSq = FMath::Square(SurfaceDistance);
			}
		}
	}

	if (OutClosestPoint)
	{
		*OutClosestPoint = ClosestPoint;
	}

	return DistanceSq;
}

void AMiningCompanionCharacter::StartPlayerInteractionMonitoring(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	RefreshPlayerInteractionState();
	if (UWorld* World = GetWorld();
		World && !World->GetTimerManager().IsTimerActive(PlayerInteractionTimerHandle))
	{
		World->GetTimerManager().SetTimer(
			PlayerInteractionTimerHandle,
			this,
			&AMiningCompanionCharacter::RefreshPlayerInteractionState,
			0.15f,
			true);
	}
}

void AMiningCompanionCharacter::RefreshPlayerInteractionState()
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (PlayerController && !IsPlayerActionLocked())
	{
		TryAutoDeposit();
	}

	const bool bActionsAvailable = PlayerController && !IsPlayerActionLocked();
	const bool bNewMiningAvailable = bActionsAvailable && FindMineableOreInRange() != nullptr;
	const bool bNewPickupAvailable = bActionsAvailable
		&& ResourceCarryComponent
		&& !ResourceCarryComponent->IsFull()
		&& FindPickupInRange() != nullptr;
	if (bMiningSkillAvailable != bNewMiningAvailable
		|| bPickupSkillAvailable != bNewPickupAvailable)
	{
		bMiningSkillAvailable = bNewMiningAvailable;
		bPickupSkillAvailable = bNewPickupAvailable;
		OnSkillAvailabilityChanged.Broadcast(bMiningSkillAvailable, bPickupSkillAvailable);
	}
}

void AMiningCompanionCharacter::TryAutoDeposit()
{
	if (!ResourceCarryComponent || ResourceCarryComponent->IsEmpty()
		|| PlayerInteractionState != EPlayerInteractionState::None)
	{
		return;
	}

	const FItemStack CarriedItem = ResourceCarryComponent->GetCurrentItem();
	AActor* Receiver = UItemLogisticsLibrary::ResolveDestination(
		this,
		CarriedItem,
		GetActorLocation());
	if (!IsValid(Receiver)
		|| FVector::DistSquared2D(
			GetActorLocation(),
			GetDeliveryPointTransform(Receiver).GetLocation())
			> FMath::Square(AutoDeliveryRadius))
	{
		return;
	}

	StartPlayerDepositAction(Receiver);
}

bool AMiningCompanionCharacter::StartPlayerCollectAction(AItemPickup* Pickup)
{
	const AMiningCompanionAIController* MiningAIConfig = GetMiningAIConfig();
	if (!Cast<APlayerController>(Controller)
		|| PlayerInteractionState != EPlayerInteractionState::None
		|| !MiningAIConfig
		|| !IsValid(Pickup)
		|| !Pickup->TryReserve(this))
	{
		return false;
	}

	PlayerInteractionPickup = Pickup;
	PlayerInteractionState = EPlayerInteractionState::AligningCollect;
	LockPlayerInteraction();
	SetActorTickEnabled(true);

	return true;
}

bool AMiningCompanionCharacter::StartPlayerDepositAction(AActor* Receiver)
{
	const AMiningCompanionAIController* MiningAIConfig = GetMiningAIConfig();
	if (!Cast<APlayerController>(Controller)
		|| PlayerInteractionState != EPlayerInteractionState::None
		|| !MiningAIConfig
		|| !IsValid(Receiver)
		|| !Receiver->GetClass()->ImplementsInterface(UItemReceiver::StaticClass())
		|| !ResourceCarryComponent
		|| ResourceCarryComponent->IsEmpty())
	{
		return false;
	}

	PlayerInteractionReceiver = Receiver;
	PlayerInteractionState = EPlayerInteractionState::AligningDeposit;
	LockPlayerInteraction();
	SetActorTickEnabled(true);

	return true;
}

void AMiningCompanionCharacter::UpdatePlayerInteractionAlignment(float DeltaSeconds)
{
	const AMiningCompanionAIController* MiningAIConfig = GetMiningAIConfig();
	if (!MiningAIConfig)
	{
		FinishPlayerInteraction();
		return;
	}

	FRotator TargetRotation = GetActorRotation();
	if (PlayerInteractionState == EPlayerInteractionState::AligningCollect)
	{
		if (!IsValid(PlayerInteractionPickup))
		{
			FinishPlayerInteraction();
			return;
		}

		FVector PickupDirection = PlayerInteractionPickup->GetActorLocation() - GetActorLocation();
		PickupDirection.Z = 0.0f;
		if (!PickupDirection.IsNearlyZero())
		{
			TargetRotation = PickupDirection.Rotation();
		}
	}
	else if (PlayerInteractionState == EPlayerInteractionState::AligningDeposit)
	{
		if (!IsValid(PlayerInteractionReceiver)
			|| !ResourceCarryComponent
			|| ResourceCarryComponent->IsEmpty())
		{
			FinishPlayerInteraction();
			return;
		}

		TargetRotation = GetDeliveryPointTransform(PlayerInteractionReceiver).Rotator();
	}
	else
	{
		SetActorTickEnabled(false);
		return;
	}

	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;
	const FRotator NewRotation = FMath::RInterpConstantTo(
		GetActorRotation(),
		TargetRotation,
		DeltaSeconds,
		MiningAIConfig->GetDeliveryRotationSpeed());
	SetActorRotation(NewRotation);

	const float RemainingYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(
		NewRotation.Yaw,
		TargetRotation.Yaw));
	if (RemainingYaw > MiningAIConfig->GetDeliveryRotationTolerance())
	{
		return;
	}

	SetActorRotation(TargetRotation);
	SetActorTickEnabled(false);
	if (PlayerInteractionState == EPlayerInteractionState::AligningCollect)
	{
		PlayerInteractionState = EPlayerInteractionState::Collecting;
		if (!PlayPlayerActionMontage(
			MiningAIConfig->GetCollectMontage(),
			MiningAIConfig->GetCollectAnimationPlayRate()))
		{
			FinishPlayerInteraction();
		}
		return;
	}

	PlayerInteractionState = EPlayerInteractionState::Depositing;
	if (!PlayPlayerActionMontage(MiningAIConfig->GetDepositMontage()))
	{
		FinishPlayerInteraction();
	}
}

bool AMiningCompanionCharacter::PlayPlayerActionMontage(UAnimMontage* Montage, float PlayRate)
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !Montage)
	{
		return false;
	}

	AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(
		this,
		&AMiningCompanionCharacter::OnPlayerActionMontageNotifyBegin);
	AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(
		this,
		&AMiningCompanionCharacter::OnPlayerActionMontageNotifyBegin);

	if (AnimInstance->Montage_Play(Montage, PlayRate) <= 0.0f)
	{
		AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(
			this,
			&AMiningCompanionCharacter::OnPlayerActionMontageNotifyBegin);
		return false;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AMiningCompanionCharacter::OnPlayerActionMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	return true;
}

void AMiningCompanionCharacter::OnPlayerActionMontageNotifyBegin(
	FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	(void)BranchingPointPayload;

	const AMiningCompanionAIController* MiningAIConfig = GetMiningAIConfig();
	if (!MiningAIConfig)
	{
		return;
	}

	if (PlayerInteractionState == EPlayerInteractionState::Collecting)
	{
		if (NotifyName == MiningAIConfig->GetCollectGrabNotifyName())
		{
			if (!IsValid(PlayerInteractionPickup)
				|| !PlayerInteractionPickup->AttachToCollector(
					GetMesh(),
					MiningAIConfig->GetCollectSocketName()))
			{
				if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
				{
					AnimInstance->Montage_Stop(0.1f, MiningAIConfig->GetCollectMontage());
				}
			}
			return;
		}

		if (NotifyName == MiningAIConfig->GetCollectReleaseNotifyName())
		{
			CommitPlayerPickup();
		}
		return;
	}

	if (PlayerInteractionState == EPlayerInteractionState::Depositing
		&& NotifyName == MiningAIConfig->GetDepositNotifyName())
	{
		CommitPlayerDeposit();
	}
}

void AMiningCompanionCharacter::OnPlayerActionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)Montage;
	(void)bInterrupted;
	FinishPlayerInteraction();
}

void AMiningCompanionCharacter::CommitPlayerPickup()
{
	AItemPickup* Pickup = PlayerInteractionPickup;
	if (!IsValid(Pickup))
	{
		PlayerInteractionPickup = nullptr;
		return;
	}

	Pickup->TryCollect(this);
	if (IsValid(Pickup))
	{
		Pickup->CancelCollect(this);
	}
	PlayerInteractionPickup = nullptr;
}

void AMiningCompanionCharacter::CommitPlayerDeposit()
{
	if (!IsValid(PlayerInteractionReceiver)
		|| !ResourceCarryComponent
		|| ResourceCarryComponent->IsEmpty())
	{
		return;
	}

	const FItemStack CarriedItem = ResourceCarryComponent->GetCurrentItem();
	if (IItemReceiver::Execute_AcceptItem(PlayerInteractionReceiver, CarriedItem))
	{
		ResourceCarryComponent->ClearItems();
	}
}

void AMiningCompanionCharacter::FinishPlayerInteraction()
{
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(
			this,
			&AMiningCompanionCharacter::OnPlayerActionMontageNotifyBegin);
	}

	if (IsValid(PlayerInteractionPickup))
	{
		PlayerInteractionPickup->CancelCollect(this);
	}
	PlayerInteractionPickup = nullptr;
	PlayerInteractionReceiver = nullptr;
	PlayerInteractionState = EPlayerInteractionState::None;
	SetActorTickEnabled(false);
	UnlockPlayerInteraction();
}

void AMiningCompanionCharacter::LockPlayerInteraction()
{
	if (bPlayerInteractionMovementLocked)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		PreviousPlayerMovementMode = MovementComponent->MovementMode;
		PreviousPlayerCustomMovementMode = MovementComponent->CustomMovementMode;
		bPreviousPlayerOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
		bPreviousPlayerUseControllerDesiredRotation = MovementComponent->bUseControllerDesiredRotation;
		MovementComponent->StopMovementImmediately();
		MovementComponent->bOrientRotationToMovement = false;
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->DisableMovement();
	}

	bPlayerInteractionMovementLocked = true;
}

void AMiningCompanionCharacter::UnlockPlayerInteraction()
{
	if (!bPlayerInteractionMovementLocked)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = bPreviousPlayerOrientRotationToMovement;
		MovementComponent->bUseControllerDesiredRotation = bPreviousPlayerUseControllerDesiredRotation;
		MovementComponent->SetMovementMode(
			PreviousPlayerMovementMode,
			PreviousPlayerCustomMovementMode);
	}

	bPlayerInteractionMovementLocked = false;
}

bool AMiningCompanionCharacter::IsPlayerActionLocked() const
{
	return PlayerInteractionState != EPlayerInteractionState::None
		|| (MiningToolComponent && MiningToolComponent->IsMining());
}

const AMiningCompanionAIController* AMiningCompanionCharacter::GetMiningAIConfig() const
{
	return AIControllerClass
		? Cast<AMiningCompanionAIController>(AIControllerClass->GetDefaultObject())
		: nullptr;
}

FTransform AMiningCompanionCharacter::GetDeliveryPointTransform(AActor* Receiver) const
{
	if (const AOreProcessorMachine* Processor = Cast<AOreProcessorMachine>(Receiver))
	{
		return Processor->GetDeliveryPointWorldTransform();
	}
	if (const AResourceDepot* Depot = Cast<AResourceDepot>(Receiver))
	{
		return Depot->GetDeliveryPointWorldTransform();
	}
	return IsValid(Receiver) ? Receiver->GetActorTransform() : FTransform::Identity;
}

void AMiningCompanionCharacter::ShowNoMineableOreMessage() const
{
	if (GEngine)
	{
		// Temporary first-slice feedback: replace with the final interaction message UI.
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Yellow,
			TEXT("\u5F53\u524D\u6CA1\u6709\u53EF\u4EA4\u4E92\u7684\u77FF\u77F3"));
	}
}

void AMiningCompanionCharacter::ShowNoPickupMessage() const
{
	if (GEngine)
	{
		// Temporary first-slice feedback: replace with the final interaction message UI.
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Yellow,
			TEXT("\u5F53\u524D\u6CA1\u6709\u53EF\u62FE\u53D6\u7684\u77FF\u77F3"));
	}
}

void AMiningCompanionCharacter::ShowCarryFullMessage() const
{
	if (GEngine)
	{
		// Temporary first-slice feedback: replace with the final interaction message UI.
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Yellow,
			TEXT("\u77FF\u77F3\u5DF2\u6EE1\uFF0C\u65E0\u6CD5\u7EE7\u7EED\u62FE\u53D6"));
	}
}

void AMiningCompanionCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	if (MiningToolComponent)
	{
		// Older placed OreBuddy instances may still serialize the player's legacy
		// PickaxeRightSocket override. Normalize after instance deserialization.
		MiningToolComponent->SetMiningHitSocketName(TEXT("S_DrillTip"));
		MiningToolComponent->OnMiningHitConfirmed.RemoveDynamic(this, &AMiningCompanionCharacter::HandleMiningHitConfirmed);
		MiningToolComponent->OnMiningHitConfirmed.AddDynamic(this, &AMiningCompanionCharacter::HandleMiningHitConfirmed);
	}
}

void AMiningCompanionCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayerInteractionTimerHandle);
	}
	FinishPlayerInteraction();

	if (MiningToolComponent)
	{
		MiningToolComponent->OnMiningHitConfirmed.RemoveDynamic(this, &AMiningCompanionCharacter::HandleMiningHitConfirmed);
	}

	Super::EndPlay(EndPlayReason);
}

void AMiningCompanionCharacter::HandleMiningHitConfirmed(FVector HitLocation, FVector HitNormal)
{
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (UOreBuddyAnimInstance* OreBuddyAnimInstance = Cast<UOreBuddyAnimInstance>(CharacterMesh->GetAnimInstance()))
		{
			OreBuddyAnimInstance->TriggerMiningImpact();
		}
	}

	if (MiningImpactSystem && GetWorld())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			MiningImpactSystem,
			HitLocation,
			HitNormal.Rotation(),
			FVector::OneVector,
			true,
			true
		);
	}

	if (MiningImpactSound && GetWorld())
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), MiningImpactSound, HitLocation);
	}
}
