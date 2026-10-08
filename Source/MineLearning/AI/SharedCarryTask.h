#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MineLearning/Combat/CombatTypes.h"
#include "SharedCarryTask.generated.h"

class AHaulerCharacter;
class AItemPickup;
class UResourceCarryComponent;
class UResourceStorageComponent;
class USceneComponent;
class USharedCarryDefinition;
class AWarehouseDepot;

UENUM(BlueprintType)
enum class ESharedCarryPhase : uint8 { Gathering, Delivering, Recovering, Complete };

/** One authoritative cargo owner. Its Blueprint child supplies the box and presentation. */
UCLASS(Blueprintable)
class MINELEARNING_API ASharedCarryTask : public AActor
{
	GENERATED_BODY()
public:
	ASharedCarryTask();
	bool Start(AHaulerCharacter* First, AHaulerCharacter* Second, AItemPickup* Pickup, USharedCarryDefinition* Definition);
	void Abort();
	UFUNCTION(BlueprintPure) ESharedCarryPhase GetPhase() const { return Phase; }
	UFUNCTION(BlueprintPure) AHaulerCharacter* GetFirstWorker() const { return A.Get(); }
	UFUNCTION(BlueprintPure) AHaulerCharacter* GetSecondWorker() const { return B.Get(); }
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UResourceCarryComponent> Cargo;
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnTaskChanged;
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	bool IsWorkerValid(AHaulerCharacter* Unit) const;
	void ReleaseWorkers();
	void RecoverCargo();
	bool MoveWorker(AHaulerCharacter* Unit, FVector Goal, float Speed, float DeltaSeconds);
	FVector GetDestinationLocation() const;
	bool BuildRoute(const FVector& From, const FVector& Goal, TArray<FVector>& Route) const;
	FVector NextWaypoint(const FVector& From, TArray<FVector>& Route) const;
	TArray<FVector> FirstRoute;
	TArray<FVector> SecondRoute;
	TArray<FVector> DeliveryRoute;
	TWeakObjectPtr<AHaulerCharacter> A;
	TWeakObjectPtr<AHaulerCharacter> B;
	UPROPERTY(Transient) TObjectPtr<AItemPickup> Source;
	UPROPERTY(Transient) TObjectPtr<AActor> Destination;
	UPROPERTY(Transient) TObjectPtr<UResourceStorageComponent> DestinationStorage;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> DestinationPoint;
	UPROPERTY(Transient) TObjectPtr<USharedCarryDefinition> Config;
	UPROPERTY(Transient) TObjectPtr<AWarehouseDepot> AccessWarehouse;
	ESharedCarryPhase Phase = ESharedCarryPhase::Complete;
	float StalledSeconds = 0.f;
	float DeliveryWait = 0.f;
	float RecoveryRetry = 0.f;
	bool bRecoveringCargo = false;
};
