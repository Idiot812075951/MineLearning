#include "OreVisualComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "MineableOre.h"
#include "TimerManager.h"

UOreVisualComponent::UOreVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOreVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningOre = Cast<AMineableOre>(GetOwner());
	if (!OwningOre.IsValid())
	{
		return;
	}

	TargetMesh = OwningOre->GetOreMesh();
	if (TargetMesh.IsValid())
	{
		BaseRelativeScale = TargetMesh->GetRelativeScale3D();
	}
	OwningOre->OnOreHealthChanged.AddDynamic(this, &UOreVisualComponent::HandleOreHealthChanged);

	// The ore may have initialized before this component bound its delegate.
	// Reading the live values guarantees correct spawn-stage selection either way.
	RefreshVisualStage();
}

void UOreVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearHitPunchTimers();
	RestoreBaseScale();

	if (OwningOre.IsValid())
	{
		OwningOre->OnOreHealthChanged.RemoveDynamic(this, &UOreVisualComponent::HandleOreHealthChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void UOreVisualComponent::RefreshVisualStage()
{
	if (!OwningOre.IsValid())
	{
		return;
	}

	HandleOreHealthChanged(OwningOre->GetCurrentHealth(), OwningOre->GetMaxHealth());
}

void UOreVisualComponent::HandleOreHealthChanged(float CurrentHealth, float MaxHealth)
{
	const bool bTookDamage = bHasObservedHealth && CurrentHealth < LastObservedHealth - KINDA_SMALL_NUMBER;
	LastObservedHealth = CurrentHealth;
	bHasObservedHealth = true;

	const UStaticMesh* PreviousMesh = TargetMesh.IsValid() ? TargetMesh->GetStaticMesh() : nullptr;
	const bool bStageChanged = OwningOre.IsValid()
		? ApplyVisualStage(OwningOre->GetCurrentMiningStageIndex())
		: false;

	if (bTookDamage && bStageChanged)
	{
		UE_LOG(LogTemp, Log, TEXT("OreVisual: StageBreak %s -> %s"),
			*GetNameSafe(PreviousMesh), *GetNameSafe(TargetMesh->GetStaticMesh()));
		PlayStageBreakVisual();
	}
	else if (bTookDamage)
	{
		UE_LOG(LogTemp, Log, TEXT("OreVisual: Hit"));
		PlayHitVisual();
	}
}

bool UOreVisualComponent::ApplyVisualStage(int32 MiningStageIndex)
{
	if (!TargetMesh.IsValid())
	{
		return false;
	}

	const FOreVisualStage* BestStage = FindVisualStage(MiningStageIndex);
	const bool bStageChanged = BestStage && TargetMesh->GetStaticMesh() != BestStage->StaticMesh;
	if (bStageChanged)
	{
		TargetMesh->SetStaticMesh(BestStage->StaticMesh);
	}

	if (BestStage && BestStage->MaterialOverride && TargetMesh->GetMaterial(0) != BestStage->MaterialOverride)
	{
		TargetMesh->SetMaterial(0, BestStage->MaterialOverride);
	}

	return bStageChanged;
}

const FOreVisualStage* UOreVisualComponent::FindVisualStage(int32 MiningStageIndex) const
{
	TArray<const FOreVisualStage*> OrderedStages;
	for (const FOreVisualStage& Stage : Stages)
	{
		if (Stage.StaticMesh)
		{
			OrderedStages.Add(&Stage);
		}
	}

	OrderedStages.Sort([](const FOreVisualStage& Left, const FOreVisualStage& Right)
	{
		return Left.MinHealthRatio > Right.MinHealthRatio;
	});

	return OrderedStages.IsValidIndex(MiningStageIndex)
		? OrderedStages[MiningStageIndex]
		: (OrderedStages.IsEmpty() ? nullptr : OrderedStages.Last());
}

void UOreVisualComponent::PlayHitVisual()
{
	PlayPunch(PunchStrength, PunchOvershoot, PunchToOvershootDelay, PunchRecoveryDelay);
}

void UOreVisualComponent::PlayStageBreakVisual()
{
	PlayPunch(StageBreakPunchStrength, StageBreakPunchOvershoot,
		StageBreakPunchToOvershootDelay, StageBreakPunchRecoveryDelay);
	SpawnVisualDebris();
}

void UOreVisualComponent::PlayPunch(float InStrength, float InOvershoot,
	float InToOvershootDelay, float InRecoveryDelay)
{
	if (!TargetMesh.IsValid())
	{
		return;
	}

	ClearHitPunchTimers();
	ActivePunchOvershoot = InOvershoot;
	TargetMesh->SetRelativeScale3D(BaseRelativeScale * FMath::Max(0.0f, 1.0f - InStrength));

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.SetTimer(PunchOvershootTimerHandle, this, &UOreVisualComponent::ApplyPunchOvershoot,
		FMath::Max(0.0f, InToOvershootDelay), false);
	TimerManager.SetTimer(PunchRestoreTimerHandle, this, &UOreVisualComponent::RestoreBaseScale,
		FMath::Max(0.0f, InToOvershootDelay) + FMath::Max(0.0f, InRecoveryDelay), false);
}

void UOreVisualComponent::SpawnVisualDebris()
{
	if (!TargetMesh.IsValid() || VisualDebrisMeshes.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<UStaticMesh*> AvailableMeshes;
	for (UStaticMesh* DebrisMesh : VisualDebrisMeshes)
	{
		if (DebrisMesh)
		{
			AvailableMeshes.Add(DebrisMesh);
		}
	}

	if (AvailableMeshes.IsEmpty())
	{
		return;
	}

	const int32 MinCount = FMath::Max(0, FMath::Min(DebrisCountMin, DebrisCountMax));
	const int32 MaxCount = FMath::Max(0, FMath::Max(DebrisCountMin, DebrisCountMax));
	const float MinScale = FMath::Max(0.0f, FMath::Min(DebrisScaleMin, DebrisScaleMax));
	const float MaxScale = FMath::Max(0.0f, FMath::Max(DebrisScaleMin, DebrisScaleMax));
	const float MinImpulse = FMath::Max(0.0f, FMath::Min(DebrisImpulseMin, DebrisImpulseMax));
	const float MaxImpulse = FMath::Max(0.0f, FMath::Max(DebrisImpulseMin, DebrisImpulseMax));
	const FVector DebrisOrigin = TargetMesh->Bounds.Origin;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < FMath::RandRange(MinCount, MaxCount); ++Index)
	{
		const FVector SpawnOffset = FMath::VRand() * FMath::FRandRange(0.0f, 25.0f);
		AStaticMeshActor* DebrisActor = World->SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(), DebrisOrigin + SpawnOffset, FRotator::ZeroRotator, SpawnParameters);
		if (!DebrisActor)
		{
			continue;
		}

		DebrisActor->SetReplicates(false);
		DebrisActor->SetLifeSpan(FMath::Max(0.0f, DebrisLifetime));

		UStaticMeshComponent* DebrisComponent = DebrisActor->GetStaticMeshComponent();
		DebrisComponent->SetMobility(EComponentMobility::Movable);
		DebrisComponent->SetStaticMesh(AvailableMeshes[FMath::RandRange(0, AvailableMeshes.Num() - 1)]);
		DebrisComponent->SetWorldScale3D(FVector(FMath::FRandRange(MinScale, MaxScale)));
		DebrisComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DebrisComponent->SetCollisionObjectType(ECC_WorldDynamic);
		DebrisComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		DebrisComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		DebrisComponent->SetGenerateOverlapEvents(false);
		DebrisComponent->SetSimulatePhysics(true);

		FVector LaunchDirection = FMath::VRand();
		LaunchDirection.Z = FMath::FRandRange(0.45f, 0.75f);
		LaunchDirection.Normalize();
		DebrisComponent->AddImpulse(LaunchDirection * FMath::FRandRange(MinImpulse, MaxImpulse), NAME_None, true);
		DebrisComponent->SetPhysicsAngularVelocityInDegrees(
			FMath::VRand() * FMath::FRandRange(240.0f, 540.0f), false, NAME_None);
	}
}

void UOreVisualComponent::ApplyPunchOvershoot()
{
	if (TargetMesh.IsValid())
	{
		TargetMesh->SetRelativeScale3D(BaseRelativeScale * (1.0f + ActivePunchOvershoot));
	}
}

void UOreVisualComponent::RestoreBaseScale()
{
	if (TargetMesh.IsValid())
	{
		TargetMesh->SetRelativeScale3D(BaseRelativeScale);
	}
}

void UOreVisualComponent::ClearHitPunchTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PunchOvershootTimerHandle);
		World->GetTimerManager().ClearTimer(PunchRestoreTimerHandle);
	}
}
