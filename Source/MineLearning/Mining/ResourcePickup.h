#pragma once

#include "CoreMinimal.h"
#include "MiningTypes.h"
#include "GameFramework/Actor.h"
#include "ResourcePickup.generated.h"

class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class MINELEARNING_API AResourcePickup : public AActor
{
	GENERATED_BODY()

public:
	AResourcePickup();

	void InitializeResource(EResourceType InType, int32 InAmount);

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EResourceType ResourceType = EResourceType::Stone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Amount = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float MiningPower = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float MiningRange = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float TraceRadius = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float AttackInterval = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	float StartForwardOffset = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	float StartHeightOffset = 60.0f;
	
	
protected:
	virtual void BeginPlay() override;
	
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