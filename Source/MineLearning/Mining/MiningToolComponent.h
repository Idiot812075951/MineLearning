#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MiningTypes.h"
#include "MiningToolComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UMiningToolComponent : public UActorComponent
{
	GENERATED_BODY()


public:
	UMiningToolComponent();

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool StartMining();

	UFUNCTION(BlueprintCallable, Category="Mining")
	void HandleMiningHitNotify();

	UFUNCTION(BlueprintCallable, Category="Mining")
	bool TryMine();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Animation")
	UAnimMontage* MiningMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Animation")
	bool bLockMovementDuringMining = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	bool bIsMining = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float MiningPower = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float TraceRadius = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	float AttackInterval = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	FName HitSocketName = TEXT("PickaxeRightSocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	bool bUseOwnerMeshSocket = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Tool")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	float StartForwardOffset = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining|Melee")
	float StartHeightOffset = 60.0f;
	

private:
	double LastMineTime = -999.0;

	FTimerHandle EndMiningTimerHandle;

private:
	void EndMining();

	ACharacter* GetOwnerCharacter() const;
	USkeletalMeshComponent* GetOwnerMesh() const;
};