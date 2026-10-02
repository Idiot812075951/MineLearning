#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "AmmoInventoryComponent.generated.h"

/** Generic reserve-magazine account. No production/run/summoner knowledge. */
UCLASS()
class MINELEARNING_API UAmmoInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UAmmoInventoryComponent();
	bool ReserveMagazine();
	bool CommitMagazine();
	void CancelReservation();
	void AddMagazines(int32 Amount);
	UFUNCTION(BlueprintPure, Category="Ammo") int32 GetAvailableMagazines() const { return Magazines - (bReserved ? 1 : 0); }
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnAmmoInventoryChanged;
private:
	int32 Magazines = 0;
	bool bReserved = false;
};
