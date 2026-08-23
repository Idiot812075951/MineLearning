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
		return 0.0f;
	}

	if (ProcessingTime <= 0.0f)
	{
		return 1.0f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	const float ElapsedTime = static_cast<float>(World->GetTimeSeconds() - ProcessingStartTime);
	return FMath::Clamp(ElapsedTime / ProcessingTime, 0.0f, 1.0f);
}

float AResourceProcessor::GetDisplayProgress() const
{
	return bIsProcessing ? GetProcessingProgress() : 0.0f;
}

EItemReceiverType AResourceProcessor::GetItemReceiverType_Implementation() const
{
	return EItemReceiverType::Processor;
}

bool AResourceProcessor::CanAcceptItem_Implementation(const FItemStack& Item) const
{
	UResourceStorageComponent* StorageComponent = ResolveSourceStorage();
	return Item.ItemType == EItemType::IronOre
		&& StorageComponent
		&& StorageComponent->CanAddItem(Item);
}

bool AResourceProcessor::AcceptItem_Implementation(const FItemStack& Item)
{
	UResourceStorageComponent* StorageComponent = ResolveSourceStorage();
	return CanAcceptItem_Implementation(Item)
		&& StorageComponent
		&& StorageComponent->AddItem(Item);
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
	UnbindSourceStorageChanged();
	CancelProcessing();

	Super::EndPlay(EndPlayReason);
}

bool AResourceProcessor::TryStartProcessing()
{
	if (bIsProcessing)
	{
		return false;
	}

	UResourceStorageComponent* StorageComponent = ResolveSourceStorage();
	if (!StorageComponent || !StorageComponent->TryReserveOre(InputOrePerBatch))
	{
		return false;
	}

	bIsProcessing = true;
	ReservedOreForCurrentBatch = InputOrePerBatch;

	UWorld* World = GetWorld();
	ProcessingStartTime = World ? World->GetTimeSeconds() : 0.0;
	OnProcessingStateChanged.Broadcast(true);

	if (!World)
	{
		CompleteProcessing();
		return true;
	}

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

void AResourceProcessor::CancelProcessing()
{
	const bool bWasProcessing = bIsProcessing;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProcessingTimerHandle);
	}

	ReleaseCurrentBatchReservation();
	bIsProcessing = false;
	ProcessingStartTime = 0.0;

	if (bWasProcessing)
	{
		OnProcessingStateChanged.Broadcast(false);
	}
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
	UResourceStorageComponent* StorageComponent = ResolveSourceStorage();
	const int32 ReservedOreForCompletedBatch = ReservedOreForCurrentBatch;
	const bool bCommitted = StorageComponent
		&& ReservedOreForCompletedBatch > 0
		&& StorageComponent->CommitReservedOre(ReservedOreForCompletedBatch);

	if (!bCommitted)
	{
		ReleaseCurrentBatchReservation();
	}

	ReservedOreForCurrentBatch = 0;
	bIsProcessing = false;
	ProcessingStartTime = 0.0;
	OnProcessingStateChanged.Broadcast(false);

	if (!bCommitted)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ResourceProcessor] Complete failed: unable to commit reserved ore Processor=%s"), *GetNameSafe(this));
		return;
	}

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
		TryStartProcessing();
	}
}

void AResourceProcessor::ReleaseCurrentBatchReservation()
{
	if (ReservedOreForCurrentBatch <= 0)
	{
		return;
	}

	if (UResourceStorageComponent* StorageComponent = ResolveSourceStorage())
	{
		StorageComponent->ReleaseReservedOre(ReservedOreForCurrentBatch);
	}

	ReservedOreForCurrentBatch = 0;
}
