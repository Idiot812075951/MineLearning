#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerTransformZone.generated.h"

class APlayerController;
class APawn;
class UBoxComponent;

/** Transformation area that swaps the player between human and OreBuddy pawns. */
UCLASS()
class MINELEARNING_API APlayerTransformZone : public AActor
{
	GENERATED_BODY()

public:
	APlayerTransformZone();

	UFUNCTION(BlueprintCallable, Category="Transformation")
	bool TryTransform(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category="Transformation")
	bool TryRestoreHumanForm(APlayerController* PlayerController);

private:
	UPROPERTY(VisibleAnywhere, Category="Transformation")
	TObjectPtr<UBoxComponent> TransformArea;

	UPROPERTY(EditAnywhere, Category="Transformation")
	TSubclassOf<APawn> RobotPawnClass;

	UPROPERTY(EditAnywhere, Category="Transformation")
	TSubclassOf<APawn> HumanPawnClass;

	bool TrySwapPawn(APlayerController* PlayerController, TSubclassOf<APawn> TargetPawnClass);
};
