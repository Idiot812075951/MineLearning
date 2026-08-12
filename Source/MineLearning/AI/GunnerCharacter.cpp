#include "GunnerCharacter.h"

#include "GunnerAIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/Image.h"
#include "DrawDebugHelpers.h"
#include "Engine/Texture2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "MineLearning/Mining/MineableOre.h"
#include "MineLearning/Mining/MiningTypes.h"
#include "MineLearning/Companion/CompanionBarkComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"

AGunnerCharacter::AGunnerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AGunnerAIController::StaticClass();

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 240.0f;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(WeaponMesh);
	// SM_AK faces +X; its measured barrel tip is at X ~= 21.5 cm.
	// Keep the tracer origin just beyond the muzzle instead of using the old
	// arbitrary 35 cm placeholder, which visibly floated beside the weapon.
	MuzzlePoint->SetRelativeLocation(FVector(22.0f, -1.0f, 5.0f));

	BarkComponent = CreateDefaultSubobject<UCompanionBarkComponent>(TEXT("BarkComponent"));

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> TracerFinder(TEXT("/Game/FX/Gunner/NS_GunnerProjectile.NS_GunnerProjectile"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> MuzzleFinder(TEXT("/Game/FX/Gunner/NS_GunnerMuzzle.NS_GunnerMuzzle"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HeadshotFinder(TEXT("/Game/FX/Gunner/NS_GunnerHeadshot.NS_GunnerHeadshot"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> GoldenFinder(TEXT("/Game/FX/Gunner/NS_GunnerGoldenHeadshot.NS_GunnerGoldenHeadshot"));
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
}

bool AGunnerCharacter::TryFireAtOre(AMineableOre* TargetOre)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(TargetOre) || TargetOre->IsActorBeingDestroyed() || TargetOre->IsDestroyed() || bIsReloading)
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
	--CurrentAmmo;

	const EGunnerShotResult Result = RollShotResult();
	const FVector MuzzleLocation = GetMuzzleLocation();
	const FVector TargetLocation = CalculateShotTarget(TargetOre, Result);
	float AppliedDamage = 0.0f;

	if (Result != EGunnerShotResult::Miss)
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

	PlayFireAnimation();
	DrawDefaultShotVisual(Result, MuzzleLocation, TargetLocation);
	PlayProductionShotVisual(Result, MuzzleLocation, TargetLocation);
	SpawnHeadshotWorldFeedback(Result, TargetLocation);
	PlayShotVisuals(Result, MuzzleLocation, TargetLocation);
	OnShotResolved.Broadcast(Result, MuzzleLocation, TargetLocation, AppliedDamage);

	if (BarkComponent)
	{
		if (Result == EGunnerShotResult::GoldenHeadshot)
		{
			BarkComponent->TrySpeak(FText::FromString(TEXT("GOLDEN HEADSHOT!")), FLinearColor(1.0f, 0.55f, 0.04f));
		}
		else if (Result == EGunnerShotResult::Headshot)
		{
			BarkComponent->TrySpeak(FText::FromString(TEXT("HEADSHOT!")));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Gunner] Shot=%s Ammo=%d/%d Damage=%.1f Target=%s"),
		*UEnum::GetValueAsString(Result), CurrentAmmo, MagazineSize, AppliedDamage, *GetNameSafe(TargetOre));

	if (CurrentAmmo <= 0)
	{
		BeginReload();
	}

	return true;
}

void AGunnerCharacter::SpawnHeadshotWorldFeedback(EGunnerShotResult Result, const FVector& TargetLocation)
{
	if ((Result != EGunnerShotResult::Headshot && Result != EGunnerShotResult::GoldenHeadshot) || !GetWorld())
	{
		return;
	}

	UClass* FeedbackWidgetClass = LoadClass<UUserWidget>(
		nullptr, TEXT("/Game/UI/Gunner/WBP_GunnerHeadshotWorldFeedback.WBP_GunnerHeadshotWorldFeedback_C"));
	if (!FeedbackWidgetClass)
	{
		return;
	}

	// This component intentionally owns no ore reference and is not attached to the ore.
	// GoldenHeadshot may destroy the ore immediately, while this feedback must finish at
	// the captured world location. Owning it from Gunner also avoids spawning a bare
	// AActor and adding its root dynamically during a shot.
	const FVector FeedbackLocation = TargetLocation + FVector(0.0f, 0.0f, 55.0f);
	UWidgetComponent* WidgetComponent = NewObject<UWidgetComponent>(this);
	AddInstanceComponent(WidgetComponent);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	WidgetComponent->SetDrawSize(FVector2D(160.0f, 160.0f));
	WidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	WidgetComponent->SetTwoSided(true);
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComponent->SetWidgetClass(FeedbackWidgetClass);
	WidgetComponent->SetRelativeScale3D(FVector(0.34f));
	WidgetComponent->RegisterComponent();
	WidgetComponent->SetWorldLocation(FeedbackLocation);
	WidgetComponent->InitWidget();

	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
	{
		const FVector ToCamera = (Camera->GetCameraLocation() - FeedbackLocation).GetSafeNormal();
		WidgetComponent->SetWorldRotation(ToCamera.Rotation());
	}

	if (UUserWidget* FeedbackWidget = WidgetComponent->GetUserWidgetObject())
	{
		if (UImage* BadgeImage = Cast<UImage>(FeedbackWidget->GetWidgetFromName(TEXT("HeadshotBadge"))))
		{
			const bool bGolden = Result == EGunnerShotResult::GoldenHeadshot;
			const TCHAR* TexturePath = bGolden
				? TEXT("/Game/UI/Gunner/T_GunnerGoldenHeadshot.T_GunnerGoldenHeadshot")
				: TEXT("/Game/UI/Gunner/T_GunnerSilverHeadshot.T_GunnerSilverHeadshot");
			if (UTexture2D* BadgeTexture = LoadObject<UTexture2D>(nullptr, TexturePath))
			{
				BadgeImage->SetBrushFromTexture(BadgeTexture, true);
			}
			BadgeImage->SetColorAndOpacity(FLinearColor::White);
			BadgeImage->SetRenderScale(bGolden ? FVector2D(1.0f) : FVector2D(0.9f));
		}
	}

	TWeakObjectPtr<UWidgetComponent> WeakWidgetComponent = WidgetComponent;
	FTimerHandle CleanupTimerHandle;
	GetWorldTimerManager().SetTimer(
		CleanupTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [WeakWidgetComponent]()
		{
			if (WeakWidgetComponent.IsValid())
			{
				WeakWidgetComponent->DestroyComponent();
			}
		}),
		1.5f,
		false);
}

FVector AGunnerCharacter::GetMuzzleLocation() const
{
	return MuzzlePoint ? MuzzlePoint->GetComponentLocation() : GetActorLocation();
}

EGunnerShotResult AGunnerCharacter::RollShotResult() const
{
	const float Golden = FMath::Max(GoldenHeadshotChance, 0.0f);
	const float Head = FMath::Max(HeadshotChance, 0.0f);
	const float Body = FMath::Max(BodyShotChance, 0.0f);
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

FVector AGunnerCharacter::CalculateShotTarget(const AMineableOre* TargetOre, EGunnerShotResult Result) const
{
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

void AGunnerCharacter::PlayFireAnimation()
{
	if (!FireAnimation || !GetMesh())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(
			FireAnimation,
			FireSlotName,
			0.05f,
			0.08f,
			1.0f,
			1,
			-1.0f,
			0.0f);
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
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, MuzzleFlashSystem, Start, Rotation, FVector(0.45f), true, true);
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
	if (bIsReloading || !GetWorld())
	{
		return;
	}

	bIsReloading = true;
	OnReloadStateChanged.Broadcast(true);
	if (BarkComponent)
	{
		BarkComponent->TrySpeak(FText::FromString(TEXT("Reloading!")));
	}
	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AGunnerCharacter::CompleteReload, FMath::Max(ReloadDuration, 0.01f), false);
	UE_LOG(LogTemp, Log, TEXT("[Gunner] Reload started (%.2fs)"), ReloadDuration);
}

void AGunnerCharacter::CompleteReload()
{
	CurrentAmmo = FMath::Max(MagazineSize, 1);
	bIsReloading = false;
	OnReloadStateChanged.Broadcast(false);
	UE_LOG(LogTemp, Log, TEXT("[Gunner] Reload complete Ammo=%d/%d"), CurrentAmmo, MagazineSize);
}
