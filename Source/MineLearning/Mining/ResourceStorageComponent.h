#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceStorageComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorageChangedSignature, int32, StoredOreCount);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UResourceStorageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceStorageComponent();

	UFUNCTION(BlueprintPure, Category="Mining|Storage")
	int32 GetStoredOreCount() const { return StoredOreCount; }

	UFUNCTION(BlueprintCallable, Category="Mining|Storage")
	int32 AddOre(int32 Amount);

	UPROPERTY(BlueprintAssignable, Category="Mining|Storage")
	FOnStorageChangedSignature OnStorageChanged;

private:
	UPROPERTY(VisibleAnywhere, Category="Mining|Storage")
	int32 StoredOreCount = 0;

	void BroadcastStorageChanged();
};
