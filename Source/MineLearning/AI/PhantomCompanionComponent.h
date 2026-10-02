#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MineLearning/Combat/CombatTypes.h"
#include "PhantomCompanionComponent.generated.h"

class APawn;

UCLASS()
class MINELEARNING_API UPhantomCompanionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPhantomCompanionComponent();
	bool SpawnFor(APawn* Template, float Lifetime);
	void Clear();
	UFUNCTION(BlueprintPure, Category="Companion") APawn* GetCompanion() const { return Companion; }
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnCompanionChanged;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION() void CompanionDestroyed(AActor* Unit);
	void LifetimeExpired();
	UPROPERTY(Transient) TObjectPtr<APawn> Companion;
	FTimerHandle LifetimeTimer;
};
