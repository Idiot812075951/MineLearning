#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RobotCenterCoinDisplayComponent.generated.h"

class UInstancedStaticMeshComponent;
class UPrimitiveComponent;
class UResourceStorageComponent;
class UStaticMesh;

/**
 * Temporary checkout presentation driven directly by Robot Center CoinStorage.
 *
 * TODO: Replace this display-only component when the Robot Center purchasing
 * flow consumes Coins and spawns production orders.
 */
UCLASS(ClassGroup=(RobotCenter), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API URobotCenterCoinDisplayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URobotCenterCoinDisplayComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleStorageChanged(int32 StoredOreCount);

	void RefreshDisplay();
	UResourceStorageComponent* FindCoinStorage() const;
	UPrimitiveComponent* FindPaymentDock() const;

	UPROPERTY(EditAnywhere, Category="Robot Center|Checkout", meta=(ClampMin="1", ClampMax="48"))
	int32 MaxVisibleCoins = 24;

	UPROPERTY(EditAnywhere, Category="Robot Center|Checkout")
	TObjectPtr<UStaticMesh> CoinMesh;

	UPROPERTY(Transient)
	TObjectPtr<UResourceStorageComponent> CoinStorage;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> CoinPileVisual;
};
