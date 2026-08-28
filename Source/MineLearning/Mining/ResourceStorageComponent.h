#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "ResourceStorageComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorageChangedSignature, int32, StoredOreCount);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UResourceStorageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceStorageComponent();

	UFUNCTION(BlueprintPure, Category="Mining|Storage")
	int32 GetStoredOreCount() const;

	UFUNCTION(BlueprintPure, Category="Mining|Storage")
	int32 GetAvailableOre() const;

	UFUNCTION(BlueprintPure, Category="Item|Storage")
	int32 GetStoredItemAmount(EItemType ItemType) const;

	UFUNCTION(BlueprintPure, Category="Item|Storage")
	int32 GetTotalStoredItemCount() const;

	UFUNCTION(BlueprintPure, Category="Item|Storage")
	bool CanAddItem(const FItemStack& Item) const;

	UFUNCTION(BlueprintCallable, Category="Item|Storage")
	bool AddItem(const FItemStack& Item);

	UFUNCTION(BlueprintCallable, Category="Item|Storage")
	bool RemoveItem(const FItemStack& Item);

	UFUNCTION(BlueprintCallable, Category="Mining|Storage")
	int32 AddOre(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Mining|Storage")
	bool TryReserveOre(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Mining|Storage")
	bool CommitReservedOre(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Mining|Storage")
	bool ReleaseReservedOre(int32 Amount);

	UPROPERTY(BlueprintAssignable, Category="Mining|Storage")
	FOnStorageChangedSignature OnStorageChanged;

private:
	/** Zero means unlimited. V1 defaults to a generous but testable capacity. */
	UPROPERTY(EditAnywhere, Category="Item|Storage", meta=(ClampMin="0"))
	int32 MaxItemCapacity = 100;

	UPROPERTY(VisibleAnywhere, Category="Item|Storage")
	TMap<EItemType, int32> StoredItems;

	UPROPERTY(VisibleAnywhere, Category="Mining|Storage")
	int32 ReservedOreCount = 0;

	void BroadcastStorageChanged();
};
