#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceCarryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCarryChangedSignature, int32, Current, int32, Max);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UResourceCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceCarryComponent();

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	int32 GetCurrentOreCount() const { return CurrentOreCount; }

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	int32 GetMaxOreCount() const { return MaxOreCount; }

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	bool IsFull() const;

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	bool CanAddOre(int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category="Mining|Carry")
	int32 AddOre(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Mining|Carry")
	int32 TakeAllOre();

	UFUNCTION(BlueprintCallable, Category="Mining|Carry")
	void ClearOre();

	UPROPERTY(BlueprintAssignable, Category="Mining|Carry")
	FOnCarryChangedSignature OnCarryChanged;

private:
	UPROPERTY(VisibleAnywhere, Category="Mining|Carry")
	int32 CurrentOreCount = 0;

	UPROPERTY(EditAnywhere, Category="Mining|Carry")
	int32 MaxOreCount = 5;

	void BroadcastCarryChanged();
};
