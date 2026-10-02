#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhantomPresentationComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UUnitRetirementComponent;

UCLASS()
class MINELEARNING_API UPhantomPresentationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPhantomPresentationComponent();
	UPROPERTY(EditAnywhere) TSoftObjectPtr<UMaterialInterface> Material;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION() void Retiring();
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
	UPROPERTY(Transient) TObjectPtr<UUnitRetirementComponent> Retirement;
	float FadeRemaining = 0.f;
};
