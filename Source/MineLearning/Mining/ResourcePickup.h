#pragma once

#include "CoreMinimal.h"
#include "MiningTypes.h"
#include "GameFramework/Actor.h"
#include "ResourcePickup.generated.h"

class UStaticMeshComponent;
class UResourceCarryComponent;
class USphereComponent;
class USkeletalMeshComponent;

UCLASS()
class MINELEARNING_API AResourcePickup : public AActor
{
	GENERATED_BODY()

public:
	AResourcePickup();

	virtual void Tick(float DeltaSeconds) override;

	void InitializeResource(EResourceType InType, int32 InAmount);

	bool TryCollect(AActor* OtherActor);

	bool TryReserve(AActor* Collector);
	bool IsAvailableFor(AActor* Collector) const;
	void ReleaseReservation(AActor* Collector);
	bool AttachToCollector(USkeletalMeshComponent* CollectorMesh, FName SocketName);
	void CancelCollect(AActor* Collector);

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EResourceType ResourceType = EResourceType::Stone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Amount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Pickup", meta=(ClampMin="1.0"))
	float AttachMoveSpeed = 600.0f;

private:
	void UpdateAttachMovement(float DeltaSeconds);

	UPROPERTY()
	TWeakObjectPtr<AActor> ReservedCollector;
};
