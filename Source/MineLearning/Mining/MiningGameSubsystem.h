#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MiningGameSubsystem.generated.h"

class UMiningPlayerData;

UCLASS()
class MINELEARNING_API UMiningGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category="Mining")
	UMiningPlayerData* GetPlayerData() const { return PlayerData; }

private:
	UPROPERTY()
	UMiningPlayerData* PlayerData = nullptr;
};
