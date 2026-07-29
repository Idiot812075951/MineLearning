#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourceProcessor.generated.h"

class AResourceDepot;
class UResourceStorageComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProcessingStateChanged, bool, bIsProcessing);

UCLASS(Blueprintable)
class MINELEARNING_API AResourceProcessor : public AActor
{
	GENERATED_BODY()

public:
	AResourceProcessor();

	UFUNCTION(BlueprintCallable, Category="Mining|Processor")
	bool TryStartProcessing();

	/** Stops the current batch and returns its reserved ore to the source storage. */
	UFUNCTION(BlueprintCallable, Category="Mining|Processor")
	void CancelProcessing();

	UFUNCTION(BlueprintPure, Category="Mining|Processor")
	bool IsProcessing() const { return bIsProcessing; }

	UFUNCTION(BlueprintPure, Category="Mining|Processor")
	float GetProcessingProgress() const;

	UFUNCTION(BlueprintPure, Category="Mining|Processor")
	float GetDisplayProgress() const;

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
	double ProcessingStartTime = 0.0;
	int32 ReservedOreForCurrentBatch = 0;
};
