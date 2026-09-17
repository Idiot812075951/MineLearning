#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QGrabTestDummy.generated.h"

class USkeletalMeshComponent;
class UArrowComponent;
class UCapsuleComponent;

/** The root is the grip/head anchor, so attachment does not depend on a moving idle pose. */
UCLASS()
class MINELEARNING_API AQGrabTestDummy : public AActor
{
	GENERATED_BODY()
public:
	AQGrabTestDummy();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UArrowComponent> GrabStandPoint;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCapsuleComponent> Capsule;
};
