#include "ResourceProcessor.h"

#include "MiningGameSubsystem.h"
#include "MiningPlayerData.h"
#include "ResourceDepot.h"
#include "ResourceStorageComponent.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

AResourceProcessor::AResourceProcessor()
{
	PrimaryActorTick.bCanEverTick = false;
}

float AResourceProcessor::GetProcessingProgress() const
{
	if (!bIsProcessing)
	{
		return ProcessingProgress;
	}

	if (ProcessingTime <= 0.0f)
	{
		return 1.0f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return ProcessingProgress;
	}

	const float ElapsedTime = static_cast<float>(World->GetTimeSeconds() - ProcessingStartTime);
	return FMath::Clamp(ElapsedTime / ProcessingTime, 0.0f, 1.0f);
}

float AResourceProcessor::GetDisplayProgress() const
{
	if (bIsProcessing)
	{
		return GetProcessingProgress();
	}

	const UResourceStorageComponent* StorageComponent = ResolveSourceStorage();
	if (!StorageComponent || InputOrePerBatch <= 0)
	{
		return 0.0f;
	}

	return FMath::Clamp(
		static_cast<float>(StorageComponent->GetStoredOreCount()) / static_cast<float>(InputOrePerBatch),
		0.0f,
		1.0f
	);
}

void AResourceProcessor::BeginPlay()
{
	Super::BeginPlay();

	BindSourceStorageChanged();

	if (bAutoStart && !BoundSourceStorageComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ResourceProcessor] AutoStart enabled but no SourceStorageComponent or SourceDepot storage is configured. Processor=%s"), *GetNameSafe(this));
	}

	if (bAutoStart)
	{
		TryStartProcessing();
	}
}

void AResourceProcessor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProcessingTimerHandle);
	}

	UnbindSourceStorageChanged();

	Super::EndPlay(EndPlayReason);
}

bool AResourceProcessor::TryStartProcessing()
{
	if (bIsProcessing)
	{
		return false;
	}

	UResourceStorageComponent* StorageComponent = ResolveSourceStorage();
	if (!StorageComponent || !StorageComponent->CanConsumeOre(InputOrePerBatch))
	{
		ProcessingProgress = 0.0f;
		return false;
	}

	bIsProcessing = true;

	if (!StorageComponent->ConsumeOre(InputOrePerBatch))
	{
		bIsProcessing = false;
		ProcessingProgress = 0.0f;
		return false;
	}

	OnProcessingStateChanged.Broadcast(true);

	UWorld* World = GetWorld();
	if (!World)
	{
		CompleteProcessing();
		return true;
	}

	ProcessingStartTime = World->GetTimeSeconds();
	ProcessingProgress = 0.0f;

	if (ProcessingTime <= 0.0f)
	{
		World->GetTimerManager().SetTimerForNextTick(this, &AResourceProcessor::CompleteProcessing);
		return true;
	}

	World->GetTimerManager().SetTimer(
		ProcessingTimerHandle,
		this,
		&AResourceProcessor::CompleteProcessing,
		ProcessingTime,
		false
	);

	return true;
}

UResourceStorageComponent* AResourceProcessor::ResolveSourceStorage() const
{
	if (SourceStorageComponent)
	{
		return SourceStorageComponent;
	}

	return SourceDepot ? SourceDepot->GetStorageComponent() : nullptr;
}

void AResourceProcessor::BindSourceStorageChanged()
{
	UResourceStorageComponent* StorageComponent = ResolveSourceStorage();
	if (StorageComponent == BoundSourceStorageComponent)
	{
		return;
	}

	UnbindSourceStorageChanged();

	BoundSourceStorageComponent = StorageComponent;
	if (BoundSourceStorageComponent)
	{
		BoundSourceStorageComponent->OnStorageChanged.AddDynamic(this, &AResourceProcessor::OnSourceStorageChanged);
	}
}

void AResourceProcessor::UnbindSourceStorageChanged()
{
	if (BoundSourceStorageComponent)
	{
		BoundSourceStorageComponent->OnStorageChanged.RemoveDynamic(this, &AResourceProcessor::OnSourceStorageChanged);
		BoundSourceStorageComponent = nullptr;
	}
}

void AResourceProcessor::OnSourceStorageChanged(int32 StoredOreCount)
{
	if (bAutoStart && !bIsProcessing)
	{
		TryStartProcessing();
	}
}

void AResourceProcessor::CompleteProcessing()
{
	bIsProcessing = false;
	ProcessingProgress = 1.0f;
	OnProcessingStateChanged.Broadcast(false);

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ResourceProcessor] Complete failed: no GameInstance Processor=%s"), *GetNameSafe(this));
	}
	else if (UMiningGameSubsystem* MiningSubsystem = GameInstance->GetSubsystem<UMiningGameSubsystem>())
	{
		if (UMiningPlayerData* PlayerData = MiningSubsystem->GetPlayerData())
		{
			PlayerData->AddProcessedOre(OutputProcessedOrePerBatch);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ResourceProcessor] Complete failed: no PlayerData Processor=%s"), *GetNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ResourceProcessor] Complete failed: no MiningGameSubsystem Processor=%s"), *GetNameSafe(this));
	}

	if (bAutoStart)
	{
		if (!TryStartProcessing())
		{
			ProcessingProgress = 0.0f;
		}
	}
}
