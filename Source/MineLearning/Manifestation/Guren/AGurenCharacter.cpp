#include "AGurenCharacter.h"


#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "GurenQSkillComponent.h"
#include "MotionWarpingComponent.h"

AGurenCharacter::AGurenCharacter()
{
	QSkill = CreateDefaultSubobject<UGurenQSkillComponent>(TEXT("QSkill"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	GetCharacterMovement()->MaxFlySpeed = NormalFlySpeed;
	GetCharacterMovement()->BrakingDecelerationFlying = 2000.f;
}

void AGurenCharacter::BeginPlay()
{
	FlightEnergy = MaxFlightEnergy;
	GetCharacterMovement()->MaxFlySpeed = NormalFlySpeed;
	Super::BeginPlay();
}

void AGurenCharacter::Jump()
{
	if (QSkill->IsQActive())
	{
		return;
	}
	bFlightHeld = true;
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		Super::Jump();
	}
}

void AGurenCharacter::StopJumping()
{
	Super::StopJumping();
	bFlightHeld = false;
	bFlightExhausted = false;
	AirborneHoldTime = 0.f;
}

bool AGurenCharacter::IsFlightActive() const
{
	return GetCharacterMovement()->IsFlying();
}

void AGurenCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	FlightGraceRemaining = FMath::Max(0.f, FlightGraceRemaining - DeltaSeconds);
	if (Movement->IsFalling() && bFlightHeld && !bFlightExhausted && FlightEnergy > 0.f)
	{
		AirborneHoldTime += DeltaSeconds;
		if (AirborneHoldTime >= 0.15f)
		{
			Super::StopJumping();
			Movement->Velocity.Z = 0.f;
			Movement->SetMovementMode(MOVE_Flying);
		}
	}
	else
	{
		AirborneHoldTime = 0.f;
	}

	// Forward/strafe input sustains flight after releasing the ascent key.
	// Use input intent so residual flying velocity cannot keep flight alive forever.
	const bool bHasHorizontalInput = GetPendingMovementInputVector().SizeSquared2D() > KINDA_SMALL_NUMBER
		|| GetLastMovementInputVector().SizeSquared2D() > KINDA_SMALL_NUMBER;
	if (IsFlightActive() && !bFlightHeld && !bHasHorizontalInput && FlightGraceRemaining <= 0.f)
	{
		Movement->SetMovementMode(MOVE_Falling);
	}

	const float PreviousEnergy = FlightEnergy;
	const float Rate = IsFlightActive() ? -FlightDrainPerSecond : FlightRegenPerSecond;
	FlightEnergy = FMath::Clamp(FlightEnergy + Rate * DeltaSeconds, 0.f, MaxFlightEnergy);
	if (IsFlightActive() && FlightEnergy <= 0.f)
	{
		// Latch until release so regeneration cannot restart flight every other frame.
		bFlightExhausted = true;
		Movement->SetMovementMode(MOVE_Falling);
	}
	if (IsFlightActive() && bFlightHeld)
	{
		AddMovementInput(FVector::UpVector);
	}
	else if (IsFlightActive())
	{
		Movement->Velocity.Z = FMath::FInterpTo(Movement->Velocity.Z, 0.f, DeltaSeconds, 8.f);
	}
	if (IsFlightActive())
	{
		// Vertical acceleration has no heading; only horizontal movement should turn Guren.
		const FVector Acceleration = Movement->GetCurrentAcceleration();
		if (Acceleration.SizeSquared2D() > KINDA_SMALL_NUMBER)
		{
			FRotator Rotation = GetActorRotation();
			Rotation.Yaw = FMath::FixedTurn(Rotation.Yaw, Acceleration.Rotation().Yaw,
				Movement->GetDeltaRotation(DeltaSeconds).Yaw);
			SetActorRotation(Rotation);
		}
	}
	if (FlightEnergy != PreviousEnergy)
	{
		OnFlightStateChanged.Broadcast(FlightEnergy, MaxFlightEnergy, IsFlightActive());
	}
}

void AGurenCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	// Flying handles yaw above so the engine cannot turn pure ascent toward yaw zero.
	GetCharacterMovement()->bOrientRotationToMovement = !IsFlightActive();
	// Allow the player to hand off Space to movement without losing the new flight state.
	if (PrevMovementMode != GetCharacterMovement()->MovementMode)
	{
		FlightGraceRemaining = IsFlightActive() ? 0.5f : 0.f;
	}
	OnFlightStateChanged.Broadcast(FlightEnergy, MaxFlightEnergy, IsFlightActive());
}

void AGurenCharacter::UnPossessed()
{
	QSkill->Cancel();
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Input->RemoveMappingContext(FlightMappingContext);
			}
		}
	}
	StopJumping();
	if (IsFlightActive())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}
	Super::UnPossessed();
}

void AGurenCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AGurenCharacter::StartQSkill);

	if (UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(
			BoostAction,
			ETriggerEvent::Started,
			this,
			&AGurenCharacter::StartBoost);

		EnhancedInputComponent->BindAction(
			BoostAction,
			ETriggerEvent::Completed,
			this,
			&AGurenCharacter::StopBoost);
	}
}

void AGurenCharacter::StartBoost()
{
	GetCharacterMovement()->MaxFlySpeed = BoostFlySpeed;
}

void AGurenCharacter::StopBoost()
{
	GetCharacterMovement()->MaxFlySpeed = NormalFlySpeed;
}

void AGurenCharacter::StartQSkill()
{
	QSkill->TryCast();
}
