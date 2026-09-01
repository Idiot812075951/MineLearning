#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemReceiver.h"
#include "SellStation.generated.h"

class AItemPickup;
class USceneComponent;
class UStaticMesh;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSaleCompletedSignature,
	FItemStack,
	SoldItem,
	int32,
	GeneratedCoinAmount);

/**
 * Receives explicitly dispatched goods and converts them into collectable Coin
 * pickups. Static art and future feedback remain assembled in BP_SellStation.
 */
UCLASS(Blueprintable)
class MINELEARNING_API ASellStation : public AActor, public IItemReceiver
{
	GENERATED_BODY()

public:
	ASellStation();

	UFUNCTION(BlueprintPure, Category="Sell Station")
	USceneComponent* GetRobotApproachPoint() const { return RobotApproachPoint; }

	UFUNCTION(BlueprintPure, Category="Sell Station")
	FText GetStationDisplayName() const { return StationDisplayName; }

	virtual EItemReceiverType GetItemReceiverType_Implementation() const override;
	virtual bool CanAcceptItem_Implementation(const FItemStack& Item) const override;
	virtual bool AcceptItem_Implementation(const FItemStack& Item) override;

	UPROPERTY(BlueprintAssignable, Category="Sell Station")
	FOnSaleCompletedSignature OnSaleCompleted;

private:
	UPROPERTY(VisibleAnywhere, Category="Sell Station")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Sell Station")
	TObjectPtr<USceneComponent> ItemDisplayPoint;

	UPROPERTY(VisibleAnywhere, Category="Sell Station")
	TObjectPtr<USceneComponent> TransactionPoint;

	UPROPERTY(VisibleAnywhere, Category="Sell Station")
	TObjectPtr<USceneComponent> CoinSpawnPoint;

	UPROPERTY(VisibleAnywhere, Category="Sell Station")
	TObjectPtr<USceneComponent> RobotApproachPoint;

	UPROPERTY(EditAnywhere, Category="Sell Station")
	FText StationDisplayName;

	UPROPERTY(EditDefaultsOnly, Category="Sell Station|Output")
	TSubclassOf<AItemPickup> CoinPickupClass;

	UPROPERTY(EditDefaultsOnly, Category="Sell Station|Output")
	TObjectPtr<UStaticMesh> CoinMesh;

	bool SpawnCoinPickup(int32 CoinAmount);
};
