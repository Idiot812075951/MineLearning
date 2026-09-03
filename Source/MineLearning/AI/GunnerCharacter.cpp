#include "GunnerCharacter.h"

#include "GunnerAIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/MiningTypes.h"
#include "MineLearning/Navigation/NavigationStandards.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName WeaponMagazineSocketComponentName(TEXT("WeaponMagazineSocket"));
	const FName ReloadMagazineComponentName(TEXT("ReloadMagazineMesh"));
	constexpr float PlayerAimYawToleranceDegrees = 3.0f;

	USceneComponent* FindGunnerSceneComponent(AActor* Actor, const FName ComponentName)
	{
		if (!Actor)
		{
			return nullptr;
		}

		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (Component && Component->GetFName() == ComponentName)
			{
				return Cast<USceneComponent>(Component);
			}
		}

		return nullptr;
	}

	UStaticMeshComponent* FindGunnerMagazineComponent(AActor* Actor, const FName ComponentName)
	{
		return Cast<UStaticMeshComponent>(FindGunnerSceneComponent(Actor, ComponentName));
	}
}

AGunnerCharacter::AGunnerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AGunnerAIController::StaticClass();

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 240.0f;
	GetCharacterMovement()->MaxStepHeight = MineLearningNavigation::CharacterStepHeight;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
	MagazineMesh->SetupAttachment(WeaponMesh);
	MagazineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(WeaponMesh);
	// SM_AK faces +X; its measured barrel tip is at X ~= 21.5 cm.
	// Keep the tracer origin just beyond the muzzle instead of using the old
	// arbitrary 35 cm placeholder, which visibly floated beside the weapon.
	MuzzlePoint->SetRelativeLocation(FVector(22.0f, -1.0f, 5.0f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("PlayerCameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 520.0f;
	CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 145.0f);
	CameraBoom->SocketOffset = FVector(0.0f, 65.0f, 0.0f);
	CameraBoom->bUsePawnControlRotation = true;
	// The transformation pad is intentionally compact. Retraction there would
	// collapse this first-pass third-person view into the Gunner's body.
	CameraBoom->bDoCollisionTest = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerFollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WeaponBodyFinder(
		TEXT("/Game/MineLearning/Characters/Gunner/Weapons/AK/Meshes/SM_AK_Body.SM_AK_Body"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MagazineFinder(
		TEXT("/Game/MineLearning/Characters/Gunner/Weapons/AK/Meshes/SM_AK_Magazine.SM_AK_Magazine"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> FireMontageFinder(
		TEXT("/Game/MineLearning/Characters/Gunner/Animations/AM_Gunner_Fire.AM_Gunner_Fire"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> BurstMontageFinder(
		TEXT("/Game/MineLearning/Characters/Gunner/Animations/AM_Gunner_Burst_3Round.AM_Gunner_Burst_3Round"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> ReloadMontageFinder(
		TEXT("/Game/MineLearning/Characters/Gunner/Animations/AM_Gunner_Reload.AM_Gunner_Reload"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextFinder(
		TEXT("/Game/MineLearning/Input/IMC_Default.IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(
		TEXT("/Game/MineLearning/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(
		TEXT("/Game/MineLearning/Input/Actions/IA_Look.IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> FireActionFinder(
		TEXT("/Game/MineLearning/Input/Actions/IA_RobotSkill1.IA_RobotSkill1"));
	static ConstructorHelpers::FObjectFinder<UInputAction> SecondarySkillActionFinder(
		TEXT("/Game/MineLearning/Input/Actions/IA_RobotPickup.IA_RobotPickup"));
	WeaponMesh->SetStaticMesh(WeaponBodyFinder.Object);
	MagazineMesh->SetStaticMesh(MagazineFinder.Object);
	FireMontage = FireMontageFinder.Object;
	BurstFireMontage = BurstMontageFinder.Object;
	ReloadMontage = ReloadMontageFinder.Object;
	PlayerMappingContext = MappingContextFinder.Object;
	MoveAction = MoveActionFinder.Object;
	LookAction = LookActionFinder.Object;
	FireAction = FireActionFinder.Object;
	SecondarySkillAction = SecondarySkillActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> TracerFinder(TEXT("/Game/MineLearning/Characters/Gunner/FX/NS_GunnerProjectile.NS_GunnerProjectile"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> MuzzleFinder(TEXT("/Game/MineLearning/Characters/Gunner/FX/NS_GunnerMuzzleFlash.NS_GunnerMuzzleFlash"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HeadshotFinder(TEXT("/Game/MineLearning/Characters/Gunner/FX/NS_GunnerHeadshot.NS_GunnerHeadshot"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> GoldenFinder(TEXT("/Game/MineLearning/Characters/Gunner/FX/NS_GunnerGoldenHeadshot.NS_GunnerGoldenHeadshot"));
	TracerSystem = TracerFinder.Object;
	MuzzleFlashSystem = MuzzleFinder.Object;
	HeadshotSystem = HeadshotFinder.Object;
	GoldenHeadshotSystem = GoldenFinder.Object;
}

void AGunnerCharacter::BeginPlay()
{
	Super::BeginPlay();

	MagazineSize = FMath::Max(MagazineSize, 1);
	CurrentAmmo = MagazineSize;
	WeaponBaseRelativeRotation = WeaponMesh ? WeaponMesh->GetRelativeRotation() : FRotator::ZeroRotator;
	AttachMagazineToWeapon();
	RegisterReloadNotifyHandlers();
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);
	ConfigureControllerMode();
}

void AGunnerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
	GetWorldTimerManager().ClearTimer(PendingReloadTimerHandle);
	GetWorldTimerManager().ClearTimer(BurstRoundTimerHandle);
	GetWorldTimerManager().ClearTimer(BurstSafetyTimerHandle);
	bReloadPending = false;
	UnregisterReloadNotifyHandlers();
	Super::EndPlay(EndPlayReason);
}

void AGunnerCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePlayerAim(DeltaSeconds);
	TryResolvePendingPlayerShot();
}

void AGunnerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	ConfigureControllerMode();
}

void AGunnerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	// Possession can precede local-player viewport/input initialization. Re-run
	// the idempotent player setup once the owning client has fully restarted.
	ConfigureControllerMode();
	GetWorldTimerManager().SetTimerForNextTick(this, &AGunnerCharacter::ApplyLocalPlayerViewport);
}

void AGunnerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGunnerCharacter::Move);
	}
	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGunnerCharacter::Look);
	}
	// A Blueprint CDO can retain a removed input asset across Live Coding. Resolve
	// the canonical shared skill action at the point where the binding needs it.
	if (!IsValid(FireAction))
	{
		FireAction = LoadObject<UInputAction>(
			nullptr,
			TEXT("/Game/MineLearning/Input/Actions/IA_RobotSkill1.IA_RobotSkill1"));
	}
	if (FireAction)
	{
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AGunnerCharacter::StartPlayerFire);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AGunnerCharacter::StopPlayerAim);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Canceled, this, &AGunnerCharacter::StopPlayerAim);
	}
	if (SecondarySkillAction)
	{
		EnhancedInputComponent->BindAction(
			SecondarySkillAction,
			ETriggerEvent::Started,
			this,
			&AGunnerCharacter::StartPlayerReload);
	}
}

void AGunnerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementVector.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementVector.X);
}

void AGunnerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void AGunnerCharacter::StartPlayerFire()
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		// Holding Q turns only the Gunner toward the controller's aim. Character
		// movement never writes back to ControlRotation, so the camera is untouched;
		// releasing Q restores normal movement-facing rotation in StopPlayerAim.
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;
		Movement->RotationRate.Yaw = 240.0f;
	}

	// Tick resolves this request only after the body has faced the camera aim.
	bPlayerAimInputHeld = true;
	bPlayerShotPending = true;
	TryResolvePendingPlayerShot();
}

void AGunnerCharacter::StopPlayerAim()
{
	bPlayerAimInputHeld = false;
	// A quick Q tap still owes the player the queued shot. Keep turning until
	// that shot is resolved, then restore movement-facing rotation below.
	if (bPlayerShotPending)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate.Yaw = 360.0f;
	}
}

void AGunnerCharacter::TryResolvePendingPlayerShot()
{
	if (!bPlayerShotPending)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController)
	{
		bPlayerShotPending = false;
		return;
	}

	FVector AimOrigin;
	FRotator AimRotation;
	PlayerController->GetPlayerViewPoint(AimOrigin, AimRotation);
	const float YawError = FMath::Abs(FMath::FindDeltaAngleDegrees(
		GetActorRotation().Yaw,
		AimRotation.Yaw));
	if (YawError > PlayerAimYawToleranceDegrees)
	{
		return;
	}

	bPlayerShotPending = false;
	TryFireAtAim(AimOrigin, AimRotation.Vector());
	if (!bPlayerAimInputHeld)
	{
		StopPlayerAim();
	}
}

void AGunnerCharacter::StartPlayerReload()
{
	RequestReload();
}

void AGunnerCharacter::UpdatePlayerAim(const float DeltaSeconds)
{
	if (!Cast<APlayerController>(Controller) || !WeaponMesh)
	{
		return;
	}

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const bool bAiming = Movement && Movement->bUseControllerDesiredRotation;
	const float ControlPitch = FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch);
	const float TargetPitch = bAiming
		? FMath::Clamp(
			ControlPitch,
			FMath::Min(PlayerWeaponPitchRange.X, PlayerWeaponPitchRange.Y),
			FMath::Max(PlayerWeaponPitchRange.X, PlayerWeaponPitchRange.Y))
		: 0.0f;
	CurrentWeaponAimPitch = FMath::FInterpTo(
		CurrentWeaponAimPitch,
		TargetPitch,
		DeltaSeconds,
		PlayerWeaponAimInterpSpeed);
	WeaponMesh->SetRelativeRotation(WeaponBaseRelativeRotation + FRotator(CurrentWeaponAimPitch, 0.0f, 0.0f));
}

void AGunnerCharacter::ConfigureControllerMode()
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	const bool bPlayerControlled = PlayerController != nullptr;
	// Mouse look owns only the camera. Gunner faces movement normally and uses
	// controller-desired rotation only while the player holds Q to aim.
	bUseControllerRotationYaw = false;
	SetActorTickEnabled(bPlayerControlled);

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;
		Movement->RotationRate.Yaw = 360.0f;
	}

	if (!bPlayerControlled)
	{
		bPlayerShotPending = false;
		bPlayerAimInputHeld = false;
		CurrentWeaponAimPitch = 0.0f;
		if (WeaponMesh)
		{
			WeaponMesh->SetRelativeRotation(WeaponBaseRelativeRotation);
		}
		OnControlModeChanged.Broadcast(false);
		return;
	}

	ApplyLocalPlayerViewport();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		if (PlayerMappingContext)
		{
			Subsystem->RemoveMappingContext(PlayerMappingContext);
			Subsystem->AddMappingContext(PlayerMappingContext, 0);
		}
	}
	OnControlModeChanged.Broadcast(true);
}

void AGunnerCharacter::ApplyLocalPlayerViewport()
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (FollowCamera)
	{
		FollowCamera->SetActive(true);
	}
	PlayerController->SetViewTarget(this);
	PlayerController->bShowMouseCursor = true;

	// Match the human form exactly: free cursor until the player holds a mouse
	// button, then release camera capture again when that button is released.
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
	if (UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		// Reset the persistent viewport settings left by older Gunner builds.
		ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringMouseDown);
		ViewportClient->SetMouseLockMode(EMouseLockMode::DoNotLock);
	}

}

bool AGunnerCharacter::TryFireAtOre(AMineableOre* TargetOre)
{
	if (!IsValid(TargetOre) || TargetOre->IsActorBeingDestroyed() || TargetOre->IsDestroyed())
	{
		return false;
	}

	FShotTarget Target;
	Target.Ore = TargetOre;
	Target.AimLocation = TargetOre->GetActorLocation();
	return TryStartAttack(Target);
}

bool AGunnerCharacter::TryFireAtAim(const FVector AimOrigin, const FVector AimDirection)
{
	UWorld* World = GetWorld();
	const FVector Direction = AimDirection.GetSafeNormal();
	if (!World || Direction.IsNearlyZero())
	{
		return false;
	}

	const FVector TraceEnd = AimOrigin + Direction * FMath::Max(PlayerAimRange, 100.0f);
	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GunnerPlayerAim), true, this);
	QueryParams.AddIgnoredActor(this);

	FShotTarget Target;
	Target.bUseExactAimLocation = true;
	Target.AimLocation = TraceEnd;
	if (World->LineTraceSingleByChannel(Hit, AimOrigin, TraceEnd, ECC_Visibility, QueryParams))
	{
		Target.AimLocation = Hit.ImpactPoint;
		Target.Ore = Cast<AMineableOre>(Hit.GetActor());
	}

	return TryStartAttack(Target);
}

bool AGunnerCharacter::TryStartAttack(const FShotTarget& Target)
{
	UWorld* World = GetWorld();
	AMineableOre* TargetOre = Target.Ore.Get();
	if (!World
		|| (!Target.bUseExactAimLocation
			&& (!IsValid(TargetOre) || TargetOre->IsActorBeingDestroyed() || TargetOre->IsDestroyed()))
		|| bIsReloading
		|| bReloadPending
		|| bBurstInProgress)
	{
		return false;
	}

	const double Now = World->GetTimeSeconds();
	if (Now < NextAllowedFireTime)
	{
		return false;
	}

	if (CurrentAmmo <= 0)
	{
		BeginReload();
		return false;
	}

	NextAllowedFireTime = Now + FMath::Max(FireInterval, 0.01f);
	// AI and player forms share the same configured point/burst selection.
	const bool bUseBurst = CurrentAmmo >= 3 && FMath::FRand() < BurstChance;

	if (!bUseBurst)
	{
		UE_LOG(LogTemp, Log, TEXT("[Gunner] AttackMode=Single Ammo=%d/%d Target=%s"),
			CurrentAmmo, MagazineSize, *GetNameSafe(TargetOre));
		ResolveShot(Target, false);
		const float ShotMontageDuration = PlayFireMontage();
		if (CurrentAmmo <= 0)
		{
			QueueReloadAfterSingleShot(ShotMontageDuration);
		}
		return true;
	}

	bBurstInProgress = true;
	BurstRoundsResolved = 0;
	BurstTarget = Target;
	UE_LOG(LogTemp, Log, TEXT("[Gunner] AttackMode=Burst queued Ammo=%d/%d Target=%s NotifyFrames=2,6,10"),
		CurrentAmmo, MagazineSize, *GetNameSafe(TargetOre));
	if (!PlayBurstFireMontage())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gunner] Burst Montage unavailable; resolving the three rounds immediately"));
		while (bBurstInProgress && BurstRoundsResolved < 3)
		{
			ResolveBurstRound(TEXT("missing montage fallback"));
		}
	}
	else
	{
		// Resolve exactly on source-animation frames 2/6/10 (24 fps).  The Montage
		// asset can lose its editor-authored named notifies, so gameplay must not
		// depend on those events to spawn projectiles or muzzle effects.
		World->GetTimerManager().SetTimer(
			BurstRoundTimerHandle,
			this,
			&AGunnerCharacter::ResolveBurstTimedShot,
			4.0f / 24.0f,
			true,
			2.0f / 24.0f);

		// This is still a final recovery guard for an interrupted or disabled world timer.
		World->GetTimerManager().SetTimer(
			BurstSafetyTimerHandle,
			this,
			&AGunnerCharacter::ResolveOutstandingBurstShots,
			FMath::Max(BurstFireMontage->GetPlayLength() + MontageSafetyPadding, 0.01f),
			false);
	}

	return true;
}

bool AGunnerCharacter::RequestReload()
{
	if (CurrentAmmo >= MagazineSize || bIsReloading || bReloadPending || bBurstInProgress)
	{
		return false;
	}

	BeginReload();
	return bIsReloading;
}

void AGunnerCharacter::ResolveShot(const FShotTarget& Target, const bool bUseBurstAccuracy, const int32 BurstRoundIndex)
{
	AMineableOre* TargetOre = Target.Ore.Get();
	const bool bOreIsValid = IsValid(TargetOre)
		&& !TargetOre->IsActorBeingDestroyed()
		&& !TargetOre->IsDestroyed();
	if ((!Target.bUseExactAimLocation && !bOreIsValid) || CurrentAmmo <= 0)
	{
		return;
	}

	--CurrentAmmo;
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);

	const EGunnerShotResult Result = bOreIsValid
		? RollShotResult(bUseBurstAccuracy)
		: EGunnerShotResult::Miss;
	const FVector MuzzleLocation = GetMuzzleLocation();
	const FVector TargetLocation = CalculateShotTarget(Target, Result);
	float AppliedDamage = 0.0f;

	if (bOreIsValid && Result != EGunnerShotResult::Miss)
	{
		AppliedDamage = BaseDamage;
		if (Result == EGunnerShotResult::Headshot)
		{
			AppliedDamage *= HeadshotDamageMultiplier;
		}

		FMiningHitRequest Request;
		Request.MiningPower = AppliedDamage;
		Request.ToolEfficiency = 1.0f;
		Request.InstigatorActor = this;
		Request.HitLocation = TargetLocation;
		Request.HitNormal = (MuzzleLocation - TargetLocation).GetSafeNormal();
		if (Result == EGunnerShotResult::GoldenHeadshot)
		{
			TargetOre->ApplyFatalMiningHit(Request);
		}
		else
		{
			TargetOre->ApplyMiningHit(Request);
		}
	}

	DrawDefaultShotVisual(Result, MuzzleLocation, TargetLocation);
	PlayProductionShotVisual(Result, MuzzleLocation, TargetLocation);
	PlayShotVisuals(Result, MuzzleLocation, TargetLocation);
	OnWeaponFired.Broadcast();
	OnShotResolved.Broadcast(Result, MuzzleLocation, TargetLocation, AppliedDamage);

	const FString Mode = bUseBurstAccuracy
		? FString::Printf(TEXT("Burst %d/3"), BurstRoundIndex)
		: TEXT("Single");
	UE_LOG(LogTemp, Log, TEXT("[Gunner] %s Result=%s Ammo=%d/%d Damage=%.1f Target=%s"),
		*Mode, *UEnum::GetValueAsString(Result), CurrentAmmo, MagazineSize, AppliedDamage, *GetNameSafe(TargetOre));

}
FVector AGunnerCharacter::GetMuzzleLocation() const
{
	return MuzzlePoint ? MuzzlePoint->GetComponentLocation() : GetActorLocation();
}

EGunnerShotResult AGunnerCharacter::RollShotResult(bool bUseBurstAccuracy) const
{
	const float AccuracyScale = bUseBurstAccuracy ? 0.5f : 1.0f;
	const float BaseGolden = FMath::Max(GoldenHeadshotChance, 0.0f);
	const float BaseHead = FMath::Max(HeadshotChance, 0.0f);
	const float Golden = BaseGolden * AccuracyScale;
	const float Head = BaseHead * AccuracyScale;
	// The reduced burst headshot chance becomes an ordinary body shot, preserving
	// the configured miss chance and making each special-hit probability exactly half.
	const float Body = FMath::Max(BodyShotChance + (BaseGolden - Golden) + (BaseHead - Head), 0.0f);
	const float Miss = FMath::Max(MissChance, 0.0f);
	const float Total = Golden + Head + Body + Miss;

	if (Total <= UE_SMALL_NUMBER)
	{
		return EGunnerShotResult::Miss;
	}

	const float Roll = FMath::FRandRange(0.0f, Total);
	if (Roll < Golden)
	{
		return EGunnerShotResult::GoldenHeadshot;
	}
	if (Roll < Golden + Head)
	{
		return EGunnerShotResult::Headshot;
	}
	if (Roll < Golden + Head + Body)
	{
		return EGunnerShotResult::BodyShot;
	}
	return EGunnerShotResult::Miss;
}

FVector AGunnerCharacter::CalculateShotTarget(const FShotTarget& Target, const EGunnerShotResult Result) const
{
	if (Target.bUseExactAimLocation)
	{
		return Target.AimLocation;
	}

	const AMineableOre* TargetOre = Target.Ore.Get();
	if (!TargetOre)
	{
		return Target.AimLocation;
	}

	FVector Origin = TargetOre->GetActorLocation();
	FVector Extent(50.0f);
	TargetOre->GetActorBounds(true, Origin, Extent);

	const FVector Jitter(
		FMath::FRandRange(-Extent.X, Extent.X) * HitJitterFraction,
		FMath::FRandRange(-Extent.Y, Extent.Y) * HitJitterFraction,
		FMath::FRandRange(-Extent.Z, Extent.Z) * HitJitterFraction);

	if (Result == EGunnerShotResult::Headshot || Result == EGunnerShotResult::GoldenHeadshot)
	{
		return Origin + FVector(0.0f, 0.0f, Extent.Z * 0.58f) + Jitter;
	}
	if (Result == EGunnerShotResult::BodyShot)
	{
		return Origin + Jitter;
	}

	FVector Away = (Origin - GetMuzzleLocation()).GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = GetActorRightVector();
	}
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Away).GetSafeNormal();
	const float SideSign = FMath::RandBool() ? 1.0f : -1.0f;
	return Origin
		+ Side * SideSign * FMath::Max(Extent.X, Extent.Y) * MissRadiusMultiplier
		+ FVector(0.0f, 0.0f, FMath::FRandRange(-0.35f, 0.75f) * Extent.Z);
}

float AGunnerCharacter::PlayFireMontage()
{
	if (!FireMontage || !GetMesh())
	{
		return 0.0f;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		return AnimInstance->Montage_Play(FireMontage);
	}

	return 0.0f;
}

bool AGunnerCharacter::PlayBurstFireMontage()
{
	if (!BurstFireMontage || !GetMesh())
	{
		return false;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		return AnimInstance->Montage_Play(BurstFireMontage) > 0.0f;
	}

	return false;
}

void AGunnerCharacter::EndBurst(const TCHAR* Reason)
{
	if (!bBurstInProgress)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Gunner] Burst complete Rounds=%d Reason=%s Ammo=%d/%d"),
		BurstRoundsResolved, Reason, CurrentAmmo, MagazineSize);
	GetWorldTimerManager().ClearTimer(BurstRoundTimerHandle);
	GetWorldTimerManager().ClearTimer(BurstSafetyTimerHandle);
	bBurstInProgress = false;
	BurstTarget = FShotTarget();
	if (CurrentAmmo <= 0)
	{
		BeginReload();
	}
}

void AGunnerCharacter::ResolveOutstandingBurstShots()
{
	if (!bBurstInProgress)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Gunner] Burst ended with %d unresolved round(s); resolving safely"), 3 - BurstRoundsResolved);
	while (bBurstInProgress && BurstRoundsResolved < 3)
	{
		ResolveBurstRound(TEXT("safety fallback"));
	}
}

void AGunnerCharacter::DrawDefaultShotVisual(EGunnerShotResult Result, const FVector& Start, const FVector& End) const
{
	if (!bDrawShotDebug || !GetWorld())
	{
		return;
	}

	const FColor Color = Result == EGunnerShotResult::GoldenHeadshot
		? FColor(255, 170, 0)
		: Result == EGunnerShotResult::Headshot
		? FColor::Yellow
		: (Result == EGunnerShotResult::BodyShot ? FColor::Orange : FColor::Red);

	DrawDebugLine(GetWorld(), Start, End, Color, false, TracerLifeSeconds, 0, TracerThickness);
	DrawDebugSphere(GetWorld(), End, Result == EGunnerShotResult::Miss ? 8.0f : 12.0f, 8, Color, false, TracerLifeSeconds);
}

void AGunnerCharacter::PlayProductionShotVisual(EGunnerShotResult Result, const FVector& Start, const FVector& End) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Direction = (End - Start).GetSafeNormal();
	const FRotator Rotation = Direction.Rotation();
	if (MuzzleFlashSystem)
	{
		// The dedicated flash is a short, bright sprite burst. Keep it large enough
		// to read clearly at normal gameplay distance without turning into a jet.
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, MuzzleFlashSystem, Start, Rotation, FVector(1.5f), true, true);
	}

	if (TracerSystem)
	{
		// Damage is already resolved by TryFireAtOre. This Niagara system is a
		// collision-free cosmetic projectile, so low frame rates or particle
		// collision misses can never change combat results.
		const float ProjectileTravelTime = FMath::Clamp(
			FVector::Distance(Start, End) / 5000.0f,
			0.09f,
			0.22f);
		const FVector ProjectileVelocity = (End - Start) / ProjectileTravelTime;
		if (UNiagaraComponent* Tracer = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, TracerSystem, Start, Rotation, FVector::OneVector, true, false))
		{
			Tracer->SetVariablePosition(TEXT("User.ProjectileStart"), Start);
			Tracer->SetVariableVec3(TEXT("User.ProjectileVelocity"), ProjectileVelocity);
			Tracer->SetVariableFloat(TEXT("User.ProjectileLifetime"), ProjectileTravelTime);
			const FLinearColor Color = Result == EGunnerShotResult::GoldenHeadshot
				? FLinearColor(1.0f, 0.52f, 0.03f, 1.0f)
				: FLinearColor(1.0f, 0.30f, 0.04f, 1.0f);
			Tracer->SetVariableLinearColor(TEXT("User.TracerColor"), Color);
			// Activate only after all world-space flight parameters are set.
			Tracer->Activate(true);
		}
	}

	UNiagaraSystem* SpecialSystem = Result == EGunnerShotResult::GoldenHeadshot
		? GoldenHeadshotSystem
		: (Result == EGunnerShotResult::Headshot ? HeadshotSystem : nullptr);
	if (SpecialSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, SpecialSystem, End, (-Direction).Rotation(), FVector::OneVector, true, true);
	}
}

void AGunnerCharacter::BeginReload()
{
	bReloadPending = false;

	if (!GetWorld())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PendingReloadTimerHandle);
	if (bIsReloading)
	{
		return;
	}

	bIsReloading = true;
	OnReloadStateChanged.Broadcast(true);
	if (PlayReloadMontage())
	{
		GetWorldTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&AGunnerCharacter::ForceCompleteReload,
			FMath::Max(ReloadMontage->GetPlayLength() + MontageSafetyPadding, 0.01f),
			false);
		UE_LOG(LogTemp, Log, TEXT("[Gunner] Reload Montage started (%.3fs)"), ReloadMontage->GetPlayLength());
		return;
	}

	// Keep the existing timer only as a missing-asset/AnimInstance fallback.
	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&AGunnerCharacter::CompleteReload,
		FMath::Max(ReloadDuration, 0.01f),
		false);
	UE_LOG(LogTemp, Warning, TEXT("[Gunner] Reload animation unavailable; using %.2fs fallback"), ReloadDuration);
}

void AGunnerCharacter::QueueReloadAfterSingleShot(const float ShotMontageDuration)
{
	if (CurrentAmmo > 0 || bIsReloading || bReloadPending || !GetWorld())
	{
		return;
	}

	if (ShotMontageDuration <= KINDA_SMALL_NUMBER)
	{
		BeginReload();
		return;
	}

	// The fire and reload Montages share DefaultSlot. Starting reload from
	// ResolveShot used to let the caller play FireMontage afterwards in the same
	// frame, which immediately interrupted and effectively erased reload.
	bReloadPending = true;
	GetWorldTimerManager().SetTimer(
		PendingReloadTimerHandle,
		this,
		&AGunnerCharacter::BeginReload,
		ShotMontageDuration,
		false);
	UE_LOG(LogTemp, Log, TEXT("[Gunner] Reload queued after final single-shot Montage (%.3fs)"), ShotMontageDuration);
}

void AGunnerCharacter::CompleteReload()
{
	if (!bIsReloading)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
	AttachMagazineToWeapon();
	CurrentAmmo = FMath::Max(MagazineSize, 1);
	bIsReloading = false;
	ActiveReloadMontage = nullptr;
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);
	OnReloadStateChanged.Broadcast(false);
	UE_LOG(LogTemp, Log, TEXT("[Gunner] Reload complete Ammo=%d/%d"), CurrentAmmo, MagazineSize);
}

bool AGunnerCharacter::PlayReloadMontage()
{
	if (!ReloadMontage || !GetMesh())
	{
		return false;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return false;
	}

	// A mesh/AnimClass refresh can replace the AnimInstance after BeginPlay.
	// Rebind the reload notifies to the instance that will actually play now.
	RegisterReloadNotifyHandlers();

	ActiveReloadMontage = ReloadMontage;
	if (AnimInstance->Montage_Play(ActiveReloadMontage) <= 0.0f)
	{
		ActiveReloadMontage = nullptr;
		return false;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AGunnerCharacter::HandleReloadMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ActiveReloadMontage);
	return true;
}

void AGunnerCharacter::ForceCompleteReload()
{
	if (!bIsReloading)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Gunner] Reload Montage timeout; forcing state recovery"));
	if (ActiveReloadMontage && GetMesh())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.05f, ActiveReloadMontage);
		}
	}
	CompleteReload();
}

void AGunnerCharacter::HandleReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveReloadMontage || !bIsReloading)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Gunner] Reload montage ended Interrupted=%s"), bInterrupted ? TEXT("true") : TEXT("false"));
	CompleteReload();
}

void AGunnerCharacter::AttachMagazineToHand()
{
	if (!bIsReloading || !MagazineMesh || !GetMesh())
	{
		return;
	}

	UStaticMeshComponent* ReloadMagazineMesh = FindGunnerMagazineComponent(this, ReloadMagazineComponentName);
	if (!ReloadMagazineMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("[Gunner][Magazine] ReloadMagazineMesh is missing; cannot show reload-hand magazine"));
		return;
	}

	// The gun magazine never leaves WeaponMagazineSocket.  The reload animation
	// only swaps visibility to a second magazine that follows the existing hand socket.
	MagazineMesh->SetVisibility(false, true);
	MagazineMesh->SetHiddenInGame(true, true);
	ReloadMagazineMesh->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		MagazineHandSocketName);
	ReloadMagazineMesh->SetHiddenInGame(false, true);
	ReloadMagazineMesh->SetVisibility(true, true);

	UE_LOG(LogTemp, Log, TEXT("[Gunner][Magazine] Mag_ToHand: gun magazine hidden; hand magazine shown at %s"), *MagazineHandSocketName.ToString());
}

void AGunnerCharacter::AttachMagazineToWeapon()
{
	if (!MagazineMesh)
	{
		return;
	}

	USceneComponent* WeaponMagazineSocket = FindGunnerSceneComponent(this, WeaponMagazineSocketComponentName);
	UStaticMeshComponent* ReloadMagazineMesh = FindGunnerMagazineComponent(this, ReloadMagazineComponentName);
	if (!WeaponMagazineSocket || !ReloadMagazineMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("[Gunner][Magazine] WeaponMagazineSocket=%s ReloadMagazineMesh=%s; cannot restore gun magazine"),
			WeaponMagazineSocket ? TEXT("valid") : TEXT("missing"),
			ReloadMagazineMesh ? TEXT("valid") : TEXT("missing"));
		return;
	}

	// Defensive restoration only.  MagazineMesh is never attached to the hand.
	MagazineMesh->AttachToComponent(WeaponMagazineSocket, FAttachmentTransformRules::SnapToTargetIncludingScale);
	MagazineMesh->SetHiddenInGame(false, true);
	MagazineMesh->SetVisibility(true, true);
	ReloadMagazineMesh->SetVisibility(false, true);
	ReloadMagazineMesh->SetHiddenInGame(true, true);

	UE_LOG(LogTemp, Log, TEXT("[Gunner][Magazine] Mag_ToGun: gun magazine restored at WeaponMagazineSocket; hand magazine hidden"));
}

void AGunnerCharacter::RegisterReloadNotifyHandlers()
{
	UAnimInstance* CurrentAnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (ReloadNotifyAnimInstance.Get() == CurrentAnimInstance)
	{
		return;
	}

	UnregisterReloadNotifyHandlers();
	if (CurrentAnimInstance)
	{
		CurrentAnimInstance->AddExternalNotifyHandler(this, GET_FUNCTION_NAME_CHECKED(AGunnerCharacter, AnimNotify_Mag_ToHand));
		CurrentAnimInstance->AddExternalNotifyHandler(this, GET_FUNCTION_NAME_CHECKED(AGunnerCharacter, AnimNotify_Mag_ToGun));
		ReloadNotifyAnimInstance = CurrentAnimInstance;
	}
}

void AGunnerCharacter::UnregisterReloadNotifyHandlers()
{
	if (UAnimInstance* AnimInstance = ReloadNotifyAnimInstance.Get())
	{
		AnimInstance->RemoveExternalNotifyHandler(this, GET_FUNCTION_NAME_CHECKED(AGunnerCharacter, AnimNotify_Mag_ToHand));
		AnimInstance->RemoveExternalNotifyHandler(this, GET_FUNCTION_NAME_CHECKED(AGunnerCharacter, AnimNotify_Mag_ToGun));
	}
	ReloadNotifyAnimInstance.Reset();
}

void AGunnerCharacter::AnimNotify_Mag_ToHand()
{
	AttachMagazineToHand();
}

void AGunnerCharacter::AnimNotify_Mag_ToGun()
{
	if (bIsReloading)
	{
		AttachMagazineToWeapon();
		UE_LOG(LogTemp, Log, TEXT("[Gunner] Mag_ToGun -> WeaponMesh"));
	}
}

void AGunnerCharacter::ResolveBurstTimedShot()
{
	ResolveBurstRound(TEXT("timer"));
}

void AGunnerCharacter::ResolveBurstRound(const TCHAR* Trigger)
{
	if (!bBurstInProgress)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Gunner] Ignored burst round (%s): no active burst"), Trigger);
		return;
	}

	AMineableOre* TargetOre = BurstTarget.Ore.Get();
	if (!BurstTarget.bUseExactAimLocation
		&& (!IsValid(TargetOre) || TargetOre->IsActorBeingDestroyed() || TargetOre->IsDestroyed()))
	{
		EndBurst(TEXT("target no longer valid"));
		return;
	}

	++BurstRoundsResolved;
	UE_LOG(LogTemp, Log, TEXT("[Gunner] Burst %s received Round=%d/3"), Trigger, BurstRoundsResolved);
	ResolveShot(BurstTarget, true, BurstRoundsResolved);

	if (BurstRoundsResolved >= 3 || CurrentAmmo <= 0)
	{
		EndBurst(BurstRoundsResolved >= 3 ? TEXT("all burst rounds resolved") : TEXT("magazine exhausted"));
	}
}
