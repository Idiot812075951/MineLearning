#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RogueliteShop.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShopRangeChanged, APawn*, Pawn, bool, bNearby);

/** Only reports proximity. The run coordinator owns purchase rules; the UI owns presentation. */
UCLASS()
class MINELEARNING_API ARogueliteShop : public AActor
{
	GENERATED_BODY()
public:
	ARogueliteShop();
	static ARogueliteShop* FindNearby(const APawn* Pawn);
	UPROPERTY(BlueprintAssignable) FShopRangeChanged OnRangeChanged;
private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionArea;
	UFUNCTION() void Entered(UPrimitiveComponent* Component, AActor* Actor, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bSweep, const FHitResult& Hit);
	UFUNCTION() void Left(UPrimitiveComponent* Component, AActor* Actor, UPrimitiveComponent* OtherComponent, int32 BodyIndex);
};
