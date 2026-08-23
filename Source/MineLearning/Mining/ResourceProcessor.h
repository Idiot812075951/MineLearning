#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemReceiver.h"
#include "ResourceProcessor.generated.h"

class AResourceDepot;
class UResourceStorageComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProcessingStateChanged, bool, bIsProcessing);

UCLASS(Blueprintable)
class MINELEARNING_API AResourceProcessor : public AActor, public IItemReceiver
{
	GENERATED_BODY()

public:
	AResourceProcessor();

	UFUNCTION(BlueprintCallable, Category="Mining|Processor")
	virtual bool TryStartProcessing();

	/** Stops the current batch and returns its reserved ore to the source storage. */
	UFUNCTION(BlueprintCallable, Category="Mining|Processor")
	virtual void CancelProcessing();

	UFUNCTION(BlueprintPure, Category="Mining|Processor")
	bool IsProcessing() const { return bIsProcessing; }

	UFUNCTION(BlueprintPure, Category="Mining|Processor")
	float GetProcessingProgress() const;

	UFUNCTION(BlueprintPure, Category="Mining|Processor")
	float GetDisplayProgress() const;

	virtual EItemReceiverType GetItemReceiverType_Implementation() const override;
	virtual bool CanAcceptItem_Implementation(const FItemStack& Item) const override;
	virtual bool AcceptItem_Implementation(const FItemStack& Item) override;

	UPROPERTY(BlueprintAssignable, Category="Mining|Processor")
	FOnProcessingStateChanged OnProcessingStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Mining|Processor")
	UResourceStorageComponent* SourceStorageComponent = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Mining|Processor")
	AResourceDepot* SourceDepot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Processor")
	int32 InputOrePerBatch = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Processor")
	int32 OutputProcessedOrePerBatch = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Processor")
	float ProcessingTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Processor")
	bool bAutoStart = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining|Processor")
	bool bIsProcessing = false;

	double ProcessingStartTime = 0.0;

private:
	UResourceStorageComponent* ResolveSourceStorage() const;
	void BindSourceStorageChanged();
	void UnbindSourceStorageChanged();
	void CompleteProcessing();
	void ReleaseCurrentBatchReservation();

	UFUNCTION()
	void OnSourceStorageChanged(int32 StoredOreCount);

	UPROPERTY()
	UResourceStorageComponent* BoundSourceStorageComponent = nullptr;

	FTimerHandle ProcessingTimerHandle;
	int32 ReservedOreForCurrentBatch = 0;
};
