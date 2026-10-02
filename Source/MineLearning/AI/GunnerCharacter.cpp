#include "GunnerCharacter.h"
#include "MineLearning/Combat/WeaponRecoilComponent.h"
#include "MineLearning/Presentation/WeaponAppearanceComponent.h"
#include "MineLearning/Combat/WeaponActionComponent.h"
#include "MineLearning/Combat/AmmoInventoryComponent.h"
#include "MineLearning/Combat/CombatDamageSubsystem.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "UObject/ConstructorHelpers.h"

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
	Recoil = CreateDefaultSubobject<UWeaponRecoilComponent>(TEXT("WeaponRecoil"));
	WeaponActions = CreateDefaultSubobject<UWeaponActionComponent>(TEXT("WeaponActions"));
	CreateDefaultSubobject<UWeaponAppearanceComponent>(TEXT("WeaponAppearance"));
	UHealthComponent* Health = CreateDefaultSubobject<UHealthComponent>(TEXT("CombatHealth"));
	Health->Faction = ECombatFaction::Player;
	UCombatComponent* Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->Config = TSoftObjectPtr<UCombatConfig>(FSoftObjectPath(TEXT("/Game/MineLearning/Combat/DA_GunnerCombat.DA_GunnerCombat")));

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
	CameraBoom->SetUsingAbsoluteRotation(true);
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
	// Keep body yaw changes from rotating the camera between movement and spring-arm ticks.
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->AddTickPrerequisiteComponent(GetCharacterMovement());

	MagazineSize = FMath::Max(MagazineSize, 1);
	WeaponActions->OnWeaponStateChanged.AddUniqueDynamic(this, &AGunnerCharacter::NotifyAmmoChanged);
	WeaponActions->Initialize(MagazineSize);
	WeaponBaseRelativeRotation = WeaponMesh ? WeaponMesh->GetRelativeRotation() : FRotator::ZeroRotator;
	AttachMagazineToWeapon();
	RegisterReloadNotifyHandlers();
	NotifyAmmoChanged();
	ConfigureControllerMode();
}

void AGunnerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	WeaponActions->OnWeaponStateChanged.RemoveDynamic(this, &AGunnerCharacter::NotifyAmmoChanged);
	CancelReload();
	GetWorldTimerManager().ClearTimer(ContinuousFireTimer);
	GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
	GetWorldTimerManager().ClearTimer(PendingReloadTimerHandle);
	GetWorldTimerManager().ClearTimer(BurstRoundTimerHandle);
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
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AGunnerCharacter::StartPlayerFire);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AGunnerCharacter::StopPlayerFire);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AGunnerCharacter::ToggleFireMode);
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
		// Holding left mouse turns only the Gunner toward the controller's aim. Character
		// movement never writes back to ControlRotation, so the camera is untouched;
		// releasing left mouse restores normal movement-facing rotation in StopPlayerAim.
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;
		Movement->RotationRate.Yaw = 240.0f;
	}

	// Tick resolves this request only after the body has faced the camera aim.
	bPlayerAimInputHeld = true;
	bPlayerShotPending = true;
	TryResolvePendingPlayerShot();
}

void AGunnerCharacter::StopPlayerFire()
{
	bPlayerAimInputHeld = false;
	GetWorldTimerManager().ClearTimer(ContinuousFireTimer);
	// A quick tap still commits its first shot after body alignment.
	if (!bPlayerShotPending)
	{
		StopPlayerAim();
	}
}

void AGunnerCharacter::StopPlayerAim()
{
	bPlayerAimInputHeld = false;
	bPlayerShotPending = false;
	GetWorldTimerManager().ClearTimer(ContinuousFireTimer);
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate.Yaw = 360.f;
	}
}

void AGunnerCharacter::CancelPlayerFire()
{
	StopPlayerAim();
}

void AGunnerCharacter::ContinuePlayerFire()
{
	if (bPlayerAimInputHeld && IsPlayerControlled() && !IsWeaponBusy() && !IsBurstMode())
	{
		bPlayerShotPending = true;
		TryResolvePendingPlayerShot();
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
	const bool bFired = TryFireAtAim(AimOrigin, AimRotation.Vector());
	if (bPlayerAimInputHeld && !IsWeaponBusy() && !IsBurstMode()
		&& (bFired || GetWorld()->GetTimeSeconds() < NextAllowedFireTime))
	{
		GetWorldTimerManager().SetTimer(ContinuousFireTimer, this, &AGunnerCharacter::ContinuePlayerFire,
			FMath::Max(0.001, NextAllowedFireTime - GetWorld()->GetTimeSeconds()), false);
	}
	if (!bPlayerAimInputHeld)
	{
		StopPlayerAim();
	}
}

void AGunnerCharacter::StartPlayerReload()
{
	RequestReload();
}

bool AGunnerCharacter::HasBurstMode() const
{
	return WeaponActions->HasBurstPattern();
}

bool AGunnerCharacter::IsBurstMode() const
{
	return WeaponActions->IsBurstEnabled();
}

void AGunnerCharacter::ToggleFireMode()
{
	if (IsPlayerControlled() && WeaponActions->ToggleBurstMode())
	{
		// Changing modes never turns an already-held key into a new attack request.
		CancelPlayerFire();
	}
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
	// controller-desired rotation only while the player holds left mouse to aim.
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

}

bool AGunnerCharacter::CanTransform() const
{
	const UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	return !IsWeaponBusy() && !bPlayerShotPending
		&& !(Anim && ((FireMontage && Anim->Montage_IsActive(FireMontage))
			|| (BurstFireMontage && Anim->Montage_IsActive(BurstFireMontage))));
}

bool AGunnerCharacter::TryFireAtOre(AMineableOre* TargetOre)
{
	if (!UCombatDamageSubsystem::CanDamageTarget(this, TargetOre))
	{
		return false;
	}

	FShotTarget Target;
	Target.Actor = TargetOre;
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

	FShotTarget Target;
	Target.bUseExactAimLocation = true;
	Target.AimOrigin = AimOrigin;
	Target.AimDirection = Direction;
	return TryStartAttack(Target);
}

bool AGunnerCharacter::TryStartAttack(const FShotTarget& Target)
{
	UWorld* World = GetWorld();
	AActor* TargetOre = Target.Actor.Get();
	if (!World || !HasAuthority()
		|| (!Target.bUseExactAimLocation
			&& (!UCombatDamageSubsystem::CanDamageTarget(this, TargetOre)))
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

	if (GetCurrentAmmo() <= 0)
	{
		BeginReload();
		return false;
	}

	const UCombatComponent* Combat = FindComponentByClass<UCombatComponent>();
	ActiveHeadMultiplier = GetShotMultiplier(EGunnerShotResult::Headshot);
	ActiveGoldenMultiplier = GetShotMultiplier(EGunnerShotResult::GoldenHeadshot);
	ActiveShotProbabilities = GetShotProbabilities(false);
	// AI and player forms share the same configured point/burst selection.
	const bool bUseBurst = WeaponActions->GetRoundsPerAttack() > 1;
	const float BaseInterval = FMath::Max(0.01f, FireInterval);
	const int32 Rounds = WeaponActions->GetRoundsPerAttack();
	const float PatternInterval = BaseInterval / (bUseBurst ? FMath::Max(1.f, BurstFireRateMultiplier) : 1.f);
	const float RoundInterval = Combat ? Combat->GetAttackInterval(PatternInterval) : FMath::Max(0.2f, PatternInterval);
	const float AttackSpeed = BaseInterval / (RoundInterval * Rounds);
	const UAnimMontage* AttackMontage = bUseBurst ? BurstFireMontage : FireMontage;
	// Gameplay owns cadence. Fit recovery into that period instead of letting
	// a long animation silently override the configured weapon fire interval.
	ActiveFirePlayRate = bUseBurst ? (4.f / 24.f) / RoundInterval
		: AttackSpeed * (AttackMontage ? FMath::Max(1.f, AttackMontage->GetPlayLength() / BaseInterval) : 1.f);
	NextAllowedFireTime = Now + FMath::Max(RoundInterval * Rounds,
		bUseBurst && AttackMontage ? AttackMontage->GetPlayLength() / ActiveFirePlayRate : 0.f);

	if (!bUseBurst)
	{
		UE_LOG(LogTemp, Log, TEXT("[Gunner] AttackMode=Single Ammo=%d/%d Target=%s"),
			GetCurrentAmmo(), MagazineSize, *GetNameSafe(TargetOre));
		ResolveShot(Target, false);
		const float ShotMontageDuration = PlayFireMontage();
		if (GetCurrentAmmo() <= 0)
		{
			QueueReloadAfterSingleShot(ShotMontageDuration);
		}
		return true;
	}

	bBurstInProgress = true;
	BurstRoundsResolved = 0;
	BurstTarget = Target;
	UE_LOG(LogTemp, Log, TEXT("[Gunner] AttackMode=Burst queued Ammo=%d/%d Target=%s NotifyFrames=2,6,10"),
		GetCurrentAmmo(), MagazineSize, *GetNameSafe(TargetOre));
	PlayBurstFireMontage();
	// Gameplay keeps the same cadence even if the presentation asset is absent.
	World->GetTimerManager().SetTimer(
		BurstRoundTimerHandle, this, &AGunnerCharacter::ResolveBurstTimedShot,
		RoundInterval, true, RoundInterval * 0.5f);

	return true;
}

bool AGunnerCharacter::RequestReload()
{
	if (GetCurrentAmmo() >= GetMagazineSize() || bIsReloading || bReloadPending || bBurstInProgress)
	{
		return false;
	}

	BeginReload();
	return bIsReloading;
}

void AGunnerCharacter::ResolveShot(const FShotTarget& RequestedTarget, const bool bUseBurstAccuracy, const int32 BurstRoundIndex)
{
	FShotTarget Target = RequestedTarget;
	AActor* TargetOre = Target.Actor.Get();
	bool bOreIsValid = UCombatDamageSubsystem::CanDamageTarget(this, TargetOre);
	if ((!Target.bUseExactAimLocation && !bOreIsValid) || GetCurrentAmmo() <= 0)
	{
		return;
	}

	const FWeaponShotContext ShotContext = WeaponActions->PrepareShot();
	if (!WeaponActions->ConsumeRound())
	{
		return;
	}
	WeaponActions->CommitShot();

	if (Target.bUseExactAimLocation)
	{
		FVector Origin = Target.AimOrigin;
		FVector Direction = Target.AimDirection;
		// Each burst round follows the current aim rather than a stale target snapshot.
		if (bUseBurstAccuracy)
		{
			if (APlayerController* Player = Cast<APlayerController>(GetController()))
			{
				FRotator View;
				Player->GetPlayerViewPoint(Origin, View);
				Direction = View.Vector();
			}
		}
		Direction = Recoil->ApplyShot(Direction);
		Target.AimLocation = Origin + Direction * FMath::Max(PlayerAimRange, 10000.f);
		Target.Actor.Reset();
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(GunnerRecoilShot), true, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, Target.AimLocation, ECC_Visibility, Query))
		{
			Target.AimLocation = Hit.ImpactPoint;
			Target.Actor = Hit.GetActor();
		}
		TargetOre = Target.Actor.Get();
		bOreIsValid = UCombatDamageSubsystem::CanDamageTarget(this, TargetOre);
	}

	UCombatComponent* Combat = FindComponentByClass<UCombatComponent>();
	const bool bInRange = bOreIsValid && (!Combat || Combat->IsInAttackRange(TargetOre));
	const EGunnerShotResult Result = bOreIsValid && !bInRange ? EGunnerShotResult::OutOfRange
		: bInRange && (!Combat || !Combat->RollAIAttackMiss()) ? RollShotResult(bUseBurstAccuracy) : EGunnerShotResult::Miss;
	const FVector MuzzleLocation = GetMuzzleLocation();
	const FVector TargetLocation = CalculateShotTarget(Target, Result);
	// Capture bounds before damage: a lethal hit can destroy the target immediately.
	FVector TargetOrigin = TargetLocation;
	FVector TargetExtent = FVector::ZeroVector;
	if (bOreIsValid)
	{
		TargetOre->GetActorBounds(true, TargetOrigin, TargetExtent);
	}
	const FVector TargetTop = TargetOrigin + FVector(0.f, 0.f, TargetExtent.Z);
	float AppliedDamage = 0.0f;

	if (bOreIsValid && Result != EGunnerShotResult::Miss && Result != EGunnerShotResult::OutOfRange)
	{
		FCombatDamageRequest Request;
		Request.Source = this;
		Request.Target = TargetOre;
		Request.SkillId = TEXT("Primary");
		Request.SnapshotDamage = (Combat ? Combat->EvaluateDamage(TEXT("Primary")) : 0.f) * ShotContext.DamageMultiplier;
		Request.Multiplier = Result == EGunnerShotResult::GoldenHeadshot ? ActiveGoldenMultiplier
			: Result == EGunnerShotResult::Headshot ? ActiveHeadMultiplier : 1.f;
		Request.HitLocation = TargetLocation;
		Request.HitNormal = (MuzzleLocation - TargetLocation).GetSafeNormal();
		AppliedDamage = GetWorld()->GetSubsystem<UCombatDamageSubsystem>()->ApplyDamage(Request).AppliedDamage;
	}

	DrawDefaultShotVisual(Result, MuzzleLocation, TargetLocation);
	PlayProductionShotVisual(Result, MuzzleLocation, TargetLocation, ShotContext.ImpactScale);
	PlayShotVisuals(Result, MuzzleLocation, TargetLocation);
	OnWeaponFired.Broadcast();
	OnShotResolved.Broadcast(Result, MuzzleLocation, TargetLocation, AppliedDamage);
	if (AppliedDamage > 0.f && (Result == EGunnerShotResult::Headshot || Result == EGunnerShotResult::GoldenHeadshot))
	{
		OnCriticalHit.Broadcast(Result == EGunnerShotResult::GoldenHeadshot, IsValid(TargetOre) ? TargetOre : nullptr, TargetTop);
	}
	if (Combat)
	{
		if (Result == EGunnerShotResult::OutOfRange) { Combat->NotifyAttackOutOfRange(); }
		Combat->NotifyAttackResolved(AppliedDamage > 0.f, false);
	}

	const FString Mode = bUseBurstAccuracy
		? FString::Printf(TEXT("Burst %d/3"), BurstRoundIndex)
		: TEXT("Single");
	UE_LOG(LogTemp, Log, TEXT("[Gunner] %s Result=%s Ammo=%d/%d Damage=%.1f Target=%s"),
		*Mode, *UEnum::GetValueAsString(Result), GetCurrentAmmo(), MagazineSize, AppliedDamage, *GetNameSafe(TargetOre));

}
FVector AGunnerCharacter::GetMuzzleLocation() const
{
	return MuzzlePoint ? MuzzlePoint->GetComponentLocation() : GetActorLocation();
}

EGunnerShotResult AGunnerCharacter::RollShotResult(bool bUseBurstAccuracy) const
{
	const float AccuracyScale = bUseBurstAccuracy ? 0.5f : 1.0f;
	const float BaseGolden = ActiveShotProbabilities.Z;
	const float BaseHead = ActiveShotProbabilities.Y;
	const float Golden = BaseGolden * AccuracyScale;
	const float Head = BaseHead * AccuracyScale;
	// Accuracy is geometric. Critical outcomes never turn a valid hit into a miss.
	const float Body = ActiveShotProbabilities.X + (BaseGolden - Golden) + (BaseHead - Head);
	const float Total = Golden + Head + Body;

	if (Total <= UE_SMALL_NUMBER)
	{
		return EGunnerShotResult::BodyShot;
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
	return EGunnerShotResult::BodyShot;
}

FVector AGunnerCharacter::CalculateShotTarget(const FShotTarget& Target, const EGunnerShotResult Result) const
{
	if (Target.bUseExactAimLocation)
	{
		return Target.AimLocation;
	}

	const AActor* TargetOre = Target.Actor.Get();
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
		return AnimInstance->Montage_Play(FireMontage, ActiveFirePlayRate, EMontagePlayReturnType::Duration);
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
		return AnimInstance->Montage_Play(BurstFireMontage, ActiveFirePlayRate) > 0.0f;
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
		BurstRoundsResolved, Reason, GetCurrentAmmo(), MagazineSize);
	GetWorldTimerManager().ClearTimer(BurstRoundTimerHandle);
	bBurstInProgress = false;
	BurstTarget = FShotTarget();
	if (GetCurrentAmmo() <= 0)
	{
		BeginReload();
	}
}

void AGunnerCharacter::DrawDefaultShotVisual(EGunnerShotResult Result, const FVector& Start, const FVector& End, float ImpactScale) const
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

void AGunnerCharacter::PlayProductionShotVisual(EGunnerShotResult Result, const FVector& Start, const FVector& End, float ImpactScale) const
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
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, SpecialSystem, End, (-Direction).Rotation(), FVector(ImpactScale), true, true);
	}
	if (ImpactScale > 1.f && !SpecialSystem && Result == EGunnerShotResult::BodyShot)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HeadshotSystem, End, (-Direction).Rotation(), FVector(ImpactScale), true, true);
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
	if (UAmmoInventoryComponent* Account = GetAmmoAccount(); Account && !Account->ReserveMagazine())
	{
		return;
	}

	const UCombatComponent* Combat = FindComponentByClass<UCombatComponent>();
	ActiveReloadSpeed = Combat ? Combat->GetCastSpeedScale() : 1.f;
	bIsReloading = true;
	OnReloadStateChanged.Broadcast(true);
	if (PlayReloadMontage())
	{
		GetWorldTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&AGunnerCharacter::ForceCompleteReload,
			FMath::Max(ReloadMontage->GetPlayLength() / ActiveReloadSpeed, 0.01f),
			false);
		UE_LOG(LogTemp, Log, TEXT("[Gunner] Reload Montage started (%.3fs)"), ReloadMontage->GetPlayLength());
		return;
	}

	// Keep the existing timer only as a missing-asset/AnimInstance fallback.
	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&AGunnerCharacter::CompleteReload,
		FMath::Max(ReloadDuration / ActiveReloadSpeed, 0.01f),
		false);
	UE_LOG(LogTemp, Warning, TEXT("[Gunner] Reload animation unavailable; using %.2fs fallback"), ReloadDuration);
}

void AGunnerCharacter::QueueReloadAfterSingleShot(const float ShotMontageDuration)
{
	if (GetCurrentAmmo() > 0 || bIsReloading || bReloadPending || !GetWorld())
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
	if (AmmoAccount && !AmmoAccount->CommitMagazine())
	{
		CancelReload();
		return;
	}
	WeaponActions->CommitReload(MagazineSize);
	bIsReloading = false;
	ActiveReloadMontage = nullptr;
	OnReloadStateChanged.Broadcast(false);
	ContinuePlayerFire();
	UE_LOG(LogTemp, Log, TEXT("[Gunner] Reload complete Ammo=%d/%d"), GetCurrentAmmo(), MagazineSize);
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
	if (AnimInstance->Montage_Play(ActiveReloadMontage, ActiveReloadSpeed) <= 0.0f)
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
	CompleteReload();
}

void AGunnerCharacter::CancelReload()
{
	if (AmmoAccount)
	{
		AmmoAccount->CancelReservation();
	}
	GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
	bIsReloading = false;
	ActiveReloadMontage = nullptr;
	OnReloadStateChanged.Broadcast(false);
	AttachMagazineToWeapon();
}

void AGunnerCharacter::HandleReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveReloadMontage || !bIsReloading)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Gunner] Reload montage ended Interrupted=%s"), bInterrupted ? TEXT("true") : TEXT("false"));
	if (bInterrupted) { CancelReload(); } else { CompleteReload(); }
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

	AActor* TargetOre = BurstTarget.Actor.Get();
	if (!BurstTarget.bUseExactAimLocation
		&& (!UCombatDamageSubsystem::CanDamageTarget(this, TargetOre)))
	{
		EndBurst(TEXT("target no longer valid"));
		return;
	}

	++BurstRoundsResolved;
	UE_LOG(LogTemp, Log, TEXT("[Gunner] Burst %s received Round=%d/3"), Trigger, BurstRoundsResolved);
	ResolveShot(BurstTarget, true, BurstRoundsResolved);

	if (BurstRoundsResolved >= 3 || GetCurrentAmmo() <= 0)
	{
		EndBurst(BurstRoundsResolved >= 3 ? TEXT("all burst rounds resolved") : TEXT("magazine exhausted"));
	}
}

float AGunnerCharacter::GetShotMultiplier(EGunnerShotResult Result) const
{
	const UCombatComponent* Combat = FindComponentByClass<UCombatComponent>();
	const float Intelligence = Combat ? Combat->GetAttributes().Intelligence : 0.f;
	if (Result == EGunnerShotResult::Headshot)
	{
		return FMath::Clamp(HeadshotDamageMultiplier + Intelligence * HeadshotIntelligenceScale, 1.f, FMath::Max(1.f, HeadshotMultiplierMax));
	}
	if (Result == EGunnerShotResult::GoldenHeadshot)
	{
		return FMath::Clamp(GoldenHeadshotMultiplier + Intelligence * GoldenIntelligenceScale, 1.f, FMath::Max(1.f, GoldenMultiplierMax));
	}
	return Result == EGunnerShotResult::Miss || Result == EGunnerShotResult::OutOfRange ? 0.f : 1.f;
}

FText AGunnerCharacter::GetCombatMechanics() const
{
	const FVector4 Probabilities = GetShotProbabilities(false);
	return FText::Format(NSLOCTEXT("Combat", "GunnerMechanics", "身体 ×1 | 爆头 ×{0} | 黄金爆头 ×{1}\n能量提高爆头倍率；上限 {2} / {3}。黄金爆头不直接秒杀。\n有效命中不会随机打空：单发爆头 {4}% / 黄金 {5}%；三连发两者概率减半。\n{6}\n弹匣：1 铁锭购买 20 发；换形态保留弹药，R 消耗备用弹匣装填。"),
		FText::AsNumber(GetShotMultiplier(EGunnerShotResult::Headshot)), FText::AsNumber(GetShotMultiplier(EGunnerShotResult::GoldenHeadshot)),
		FText::AsNumber(HeadshotMultiplierMax), FText::AsNumber(GoldenMultiplierMax),
		FText::AsNumber(Probabilities.Y * 100.f),
		FText::AsNumber(Probabilities.Z * 100.f),
		GetAmmoStatusText());
}

FVector4 AGunnerCharacter::GetShotProbabilities(bool bBurst) const
{
	FVector4 Values(0.f, FMath::Max(0.f, HeadshotChance), FMath::Max(0.f, GoldenHeadshotChance), 0.f);
	const double CriticalTotal = Values.Y + Values.Z;
	if (CriticalTotal > 1.0) { Values /= CriticalTotal; }
	Values.X = FMath::Max(0.0, 1.0 - Values.Y - Values.Z);
	const UCombatComponent* Combat = FindComponentByClass<UCombatComponent>();
	const double Transfer = FMath::Clamp(static_cast<double>(Combat ? Combat->GetModifiers().GoldenProbability : 0.f), -Values.Z, Values.X);
	Values.X -= Transfer;
	Values.Z += Transfer;
	if (bBurst)
	{
		Values.X += (Values.Y + Values.Z) * 0.5;
		Values.Y *= 0.5;
		Values.Z *= 0.5;
	}
	return Values;
}

UAmmoInventoryComponent* AGunnerCharacter::GetAmmoAccount() const
{
	return AmmoAccount;
}

void AGunnerCharacter::SetAmmoAccount(UAmmoInventoryComponent* Account)
{
	AmmoAccount = Account;
}

bool AGunnerCharacter::TryLoadEmptyMagazine()
{
	if (!HasAuthority() || GetCurrentAmmo() > 0 || IsWeaponBusy()
		|| !AmmoAccount || !AmmoAccount->ReserveMagazine())
	{
		return false;
	}
	if (!AmmoAccount->CommitMagazine())
	{
		AmmoAccount->CancelReservation();
		return false;
	}
	// Use the ordinary reload commit so magazine-capacity effects still apply.
	WeaponActions->CommitReload(MagazineSize);
	return true;
}

void AGunnerCharacter::NotifyAmmoChanged()
{
	OnAmmoChanged.Broadcast(GetCurrentAmmo(), GetMagazineSize());
}

int32 AGunnerCharacter::GetCurrentAmmo() const
{
	return WeaponActions->GetCurrentAmmo();
}

int32 AGunnerCharacter::GetMagazineSize() const
{
	return WeaponActions->GetCapacity();
}

float AGunnerCharacter::GetEffectiveAttackRange() const
{
	const UCombatComponent* Combat = FindComponentByClass<UCombatComponent>();
	return Combat ? Combat->GetAttackRange() : 900.f;
}

void AGunnerCharacter::RestoreLoadedAmmo(int32 Ammo)
{
	FWeaponRuntimeState State = WeaponActions->Export();
	State.Ammo = FMath::Clamp(Ammo, 0, State.Capacity);
	WeaponActions->Restore(State);
}

FText AGunnerCharacter::GetAmmoStatusText() const
{
	const UAmmoInventoryComponent* Account = GetAmmoAccount();
	return FText::Format(NSLOCTEXT("Gunner", "AmmoBudget", "子弹 {0}/{1} · 备用弹匣 {2}"),
		FText::AsNumber(GetCurrentAmmo()), FText::AsNumber(GetMagazineSize()),
		Account ? FText::AsNumber(Account->GetAvailableMagazines()) : FText::FromString(TEXT("AI")));
}
