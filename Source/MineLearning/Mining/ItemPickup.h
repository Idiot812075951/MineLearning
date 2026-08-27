#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemTypes.h"
#include "ItemPickup.generated.h"

class USkeletalMeshComponent;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class MINELEARNING_API AItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AItemPickup();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	void InitializeItem(
		const FItemStack& InItemStack,
		const TArray<TObjectPtr<UStaticMesh>>& InDropMeshes,
		float InDropMeshScale
	);

	UFUNCTION(BlueprintPure, Category="Item|Pickup")
	const FItemStack& GetItemStack() const { return ItemStack; }

	UFUNCTION(BlueprintCallable, Category="Item|Pickup")
	void SetItemStack(const FItemStack& InItemStack);

	UFUNCTION(BlueprintPure, Category="Item|Pickup")
	int32 GetAmount() const { return ItemStack.Amount; }

	UFUNCTION(BlueprintCallable, Category="Item|Pickup")
	virtual bool TryCollect(AActor* OtherActor);

	/** Prevents logistics workers and physics from taking over while a pickup is travelling on a machine path. */
	UFUNCTION(BlueprintCallable, Category="Item|Pickup")
	void SetTransportLocked(bool bLocked);

	/** Makes the pickup collectable at an authored station point without gravity. */
	void ReleaseStationaryForCollection();

	UFUNCTION(BlueprintPure, Category="Item|Pickup")
	bool IsTransportLocked() const { return bTransportLocked; }

	bool TryReserve(AActor* Collector);
	bool IsAvailableFor(AActor* Collector) const;
	void ReleaseReservation(AActor* Collector);
	bool AttachToCollector(USkeletalMeshComponent* CollectorMesh, FName SocketName);
	void CancelCollect(AActor* Collector);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Pickup")
	FItemStack ItemStack = {EItemType::IronOre, 1};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item|Pickup")
	TObjectPtr<UStaticMesh> SelectedDropMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Pickup", meta=(ClampMin="1.0"))
	float AttachMoveSpeed = 600.0f;

protected:
	void SelectDropMesh(const TArray<TObjectPtr<UStaticMesh>>& InDropMeshes, float InDropMeshScale);

private:
	void UpdateAttachMovement(float DeltaSeconds);

	UPROPERTY()
	TWeakObjectPtr<AActor> ReservedCollector;

	float ReservedResourceMeshScale = 1.0f;

	UPROPERTY(VisibleAnywhere, Category="Item|Pickup")
	bool bTransportLocked = false;
};
