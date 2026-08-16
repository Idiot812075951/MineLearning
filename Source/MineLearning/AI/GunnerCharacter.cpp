#include "GunnerCharacter.h"

#include "GunnerAIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Components/Image.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
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

namespace
{
	const FName WeaponMagazineSocketComponentName(TEXT("WeaponMagazineSocket"));
	const FName ReloadMagazineComponentName(TEXT("ReloadMagazineMesh"));

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

	MagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
	MagazineMesh->SetupAttachment(WeaponMesh);
	MagazineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(WeaponMesh);
	// SM_AK faces +X; its measured barrel tip is at X ~= 21.5 cm.
	// Keep the tracer origin just beyond the muzzle instead of using the old
	// arbitrary 35 cm placeholder, which visibly floated beside the weapon.
	MuzzlePoint->SetRelativeLocation(FVector(22.0f, -1.0f, 5.0f));

	BarkComponent = CreateDefaultSubobject<UCompanionBarkComponent>(TEXT("BarkComponent"));

	AmmoWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("AmmoWidget"));
	AmmoWidgetComponent->SetupAttachment(GetRootComponent());
	AmmoWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 11.0f));
	AmmoWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	AmmoWidgetComponent->SetDrawSize(FVector2D(160.0f, 40.0f));
	AmmoWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	AmmoWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WeaponBodyFinder(
		TEXT("/Game/Resource/Robot/Gunner/AK/SM_AK_Body.SM_AK_Body"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MagazineFinder(
		TEXT("/Game/Resource/Robot/Gunner/AK/SM_AK_Magazine.SM_AK_Magazine"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> FireMontageFinder(
		TEXT("/Game/Resource/Robot/Gunner/AM_Gunner_Fire.AM_Gunner_Fire"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> BurstMontageFinder(
		TEXT("/Game/Resource/Robot/Gunner/AM_Gunner_Burst_3Round.AM_Gunner_Burst_3Round"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> ReloadMontageFinder(
		TEXT("/Game/Resource/Robot/Gunner/AM_Gunner_Reload.AM_Gunner_Reload"));
	static ConstructorHelpers::FClassFinder<UUserWidget> AmmoWidgetFinder(
		TEXT("/Game/UI/Companion/WBP_CompanionBark"));
	WeaponMesh->SetStaticMesh(WeaponBodyFinder.Object);
	MagazineMesh->SetStaticMesh(MagazineFinder.Object);
	FireMontage = FireMontageFinder.Object;
	BurstFireMontage = BurstMontageFinder.Object;
	ReloadMontage = ReloadMontageFinder.Object;
	AmmoWidgetComponent->SetWidgetClass(AmmoWidgetFinder.Class);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> TracerFinder(TEXT("/Game/FX/Gunner/NS_GunnerProjectile.NS_GunnerProjectile"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> MuzzleFinder(TEXT("/Game/FX/Gunner/NS_GunnerMuzzleFlash.NS_GunnerMuzzleFlash"));
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
	AttachMagazineToWeapon();
#if WITH_EDITOR
	EnsureBurstMontageNotifies();
#endif
	RegisterReloadNotifyHandlers();
	UpdateAmmoDisplay();
}

void AGunnerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterReloadNotifyHandlers();
	Super::EndPlay(EndPlayReason);
}

bool AGunnerCharacter::TryFireAtOre(AMineableOre* TargetOre)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(TargetOre) || TargetOre->IsActorBeingDestroyed() || TargetOre->IsDestroyed() || bIsReloading || bBurstInProgress)
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
	// Each attack is independently a 50/50 point-shot or three-round burst.
	// A burst is never selected when it cannot spend all three rounds.
	const bool bUseBurst = CurrentAmmo >= 3 && FMath::FRand() < BurstChance;

	if (!bUseBurst)
	{
		UE_LOG(LogTemp, Log, TEXT("[Gunner] AttackMode=Single Ammo=%d/%d Target=%s"),
			CurrentAmmo, MagazineSize, *GetNameSafe(TargetOre));
		ResolveShot(TargetOre, false);
		PlayFireMontage();
		return true;
	}

	bBurstInProgress = true;
	BurstRoundsResolved = 0;
	BurstTargetOre = TargetOre;
	UE_LOG(LogTemp, Log, TEXT("[Gunner] AttackMode=Burst queued Ammo=%d/%d Target=%s NotifyFrames=2,6,10"),
		CurrentAmmo, MagazineSize, *GetNameSafe(TargetOre));
	if (!PlayBurstFireMontage())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gunner] Burst Montage unavailable; resolving the three rounds immediately"));
		while (bBurstInProgress && BurstRoundsResolved < 3)
		{
			AnimNotify_GunnerBurstShot();
		}
	}
	else
	{
		// Resolve exactly on source-animation frames 2/6/10 (24 fps).  The Montage
		// asset can lose its editor-authored named notifies, so gameplay must not
		// depend on those events to spawn projectiles or muzzle effects.
		bBurstTimingDriven = true;
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

void AGunnerCharacter::ResolveShot(AMineableOre* TargetOre, bool bUseBurstAccuracy, int32 BurstRoundIndex)
{
	if (!IsValid(TargetOre) || TargetOre->IsActorBeingDestroyed() || TargetOre->IsDestroyed() || CurrentAmmo <= 0)
	{
		return;
	}

	--CurrentAmmo;
	UpdateAmmoDisplay();

	const EGunnerShotResult Result = RollShotResult(bUseBurstAccuracy);
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
		else if (Result == EGunnerShotResult::Miss)
		{
			BarkComponent->TrySpeak(FText::FromString(TEXT("I MISSED!")), FLinearColor(1.0f, 0.4f, 0.08f), false, true);
		}
	}

	const FString Mode = bUseBurstAccuracy
		? FString::Printf(TEXT("Burst %d/3"), BurstRoundIndex)
		: TEXT("Single");
	UE_LOG(LogTemp, Log, TEXT("[Gunner] %s Result=%s Ammo=%d/%d Damage=%.1f Target=%s"),
		*Mode, *UEnum::GetValueAsString(Result), CurrentAmmo, MagazineSize, AppliedDamage, *GetNameSafe(TargetOre));

	if (CurrentAmmo <= 0)
	{
		// A burst finishes its current animation before the normal reload flow starts.
		if (!bBurstInProgress)
		{
			BeginReload();
		}
	}
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

void AGunnerCharacter::PlayFireMontage()
{
	if (!FireMontage || !GetMesh())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(FireMontage);
	}
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

#if WITH_EDITOR
void AGunnerCharacter::EnsureBurstMontageNotifies()
{
	if (!BurstFireMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("[Gunner] Burst Montage notify setup failed: Montage is missing"));
		return;
	}

	// These three moments correspond to source-animation frames 2, 6, and 10
	// at its fixed 24 fps import rate. They must live on the Montage itself:
	// source-sequence named notifies are not propagated by this Montage asset.
	constexpr float NotifyTimes[] = { 2.0f / 24.0f, 6.0f / 24.0f, 10.0f / 24.0f };
	int32 AddedCount = 0;
	for (const float NotifyTime : NotifyTimes)
	{
		const bool bAlreadyExists = BurstFireMontage->Notifies.ContainsByPredicate(
			[NotifyTime](const FAnimNotifyEvent& Notify)
			{
				return Notify.NotifyName == TEXT("GunnerBurstShot")
					&& FMath::IsNearlyEqual(Notify.GetTime(), NotifyTime, KINDA_SMALL_NUMBER);
			});
		if (bAlreadyExists)
		{
			continue;
		}

		FAnimNotifyEvent& NewNotify = BurstFireMontage->Notifies.AddDefaulted_GetRef();
		NewNotify.NotifyName = TEXT("GunnerBurstShot");
		NewNotify.Link(BurstFireMontage, NotifyTime);
		++AddedCount;
	}

	if (AddedCount > 0)
	{
		BurstFireMontage->MarkPackageDirty();
		UE_LOG(LogTemp, Log, TEXT("[Gunner] Added %d persistent GunnerBurstShot Montage notifies at frames 2, 6, 10"), AddedCount);
	}
}
#endif

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
	bBurstTimingDriven = false;
	bBurstInProgress = false;
	BurstTargetOre.Reset();
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
	if (bIsReloading || !GetWorld())
	{
		return;
	}

	bIsReloading = true;
	OnReloadStateChanged.Broadcast(true);
	if (BarkComponent)
	{
		// Reload feedback must not be suppressed by a headshot bark on the final round.
		BarkComponent->TrySpeak(FText::FromString(TEXT("Reloading!")), FLinearColor::White, false, true);
	}
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
	UpdateAmmoDisplay();
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

void AGunnerCharacter::UpdateAmmoDisplay()
{
	if (!AmmoWidgetComponent)
	{
		return;
	}

	AmmoWidgetComponent->InitWidget();
	if (UUserWidget* Widget = AmmoWidgetComponent->GetUserWidgetObject())
	{
		if (UTextBlock* AmmoText = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("BarkText"))))
		{
			AmmoText->SetText(FText::Format(
				NSLOCTEXT("Gunner", "AmmoFormat", "{0}/{1}"),
				FText::AsNumber(CurrentAmmo),
				FText::AsNumber(MagazineSize)));
			AmmoText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.95f, 1.0f, 1.0f)));
			AmmoText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	AmmoWidgetComponent->SetVisibility(true);
}

void AGunnerCharacter::RegisterReloadNotifyHandlers()
{
	if (GetMesh())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->AddExternalNotifyHandler(this, GET_FUNCTION_NAME_CHECKED(AGunnerCharacter, AnimNotify_Mag_ToHand));
			AnimInstance->AddExternalNotifyHandler(this, GET_FUNCTION_NAME_CHECKED(AGunnerCharacter, AnimNotify_Mag_ToGun));
			AnimInstance->AddExternalNotifyHandler(this, GET_FUNCTION_NAME_CHECKED(AGunnerCharacter, AnimNotify_GunnerBurstShot));
		}
	}
}

void AGunnerCharacter::UnregisterReloadNotifyHandlers()
{
	if (GetMesh())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->RemoveExternalNotifyHandler(this, GET_FUNCTION_NAME_CHECKED(AGunnerCharacter, AnimNotify_Mag_ToHand));
			AnimInstance->RemoveExternalNotifyHandler(this, GET_FUNCTION_NAME_CHECKED(AGunnerCharacter, AnimNotify_Mag_ToGun));
			AnimInstance->RemoveExternalNotifyHandler(this, GET_FUNCTION_NAME_CHECKED(AGunnerCharacter, AnimNotify_GunnerBurstShot));
		}
	}
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

void AGunnerCharacter::AnimNotify_GunnerBurstShot()
{
	if (bBurstTimingDriven)
	{
		// Timed resolution is authoritative. Ignore a duplicate named notify if
		// the asset is later repaired or reimported with one.
		UE_LOG(LogTemp, Verbose, TEXT("[Gunner] Ignored duplicate GunnerBurstShot notify during timer-driven burst"));
		return;
	}

	ResolveBurstRound(TEXT("animation notify"));
}

void AGunnerCharacter::ResolveBurstTimedShot()
{
	ResolveBurstRound(TEXT("animation timing"));
}

void AGunnerCharacter::ResolveBurstRound(const TCHAR* Trigger)
{
	if (!bBurstInProgress)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Gunner] Ignored burst round (%s): no active burst"), Trigger);
		return;
	}

	AMineableOre* TargetOre = BurstTargetOre.Get();
	if (!IsValid(TargetOre) || TargetOre->IsActorBeingDestroyed() || TargetOre->IsDestroyed())
	{
		EndBurst(TEXT("target no longer valid"));
		return;
	}

	++BurstRoundsResolved;
	UE_LOG(LogTemp, Log, TEXT("[Gunner] Burst %s received Round=%d/3"), Trigger, BurstRoundsResolved);
	ResolveShot(TargetOre, true, BurstRoundsResolved);

	if (BurstRoundsResolved >= 3 || CurrentAmmo <= 0)
	{
		EndBurst(BurstRoundsResolved >= 3 ? TEXT("all burst rounds resolved") : TEXT("magazine exhausted"));
	}
}
