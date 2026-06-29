#pragma once

#include "CoreMinimal.h"
#include "MiningTypes.h"
#include "GameFramework/Actor.h"
#include "ResourcePickup.generated.h"

class UStaticMeshComponent;
class UResourceCarryComponent;
class USphereComponent;

UCLASS()
class MINELEARNING_API AResourcePickup : public AActor
{
	GENERATED_BODY()

public:
	AResourcePickup();

	void InitializeResource(EResourceType InType, int32 InAmount);

	bool TryCollect(AActor* OtherActor);

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EResourceType ResourceType = EResourceType::Stone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Amount = 1;

protected:
	UFUNCTION()
	void OnPickupSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
};
