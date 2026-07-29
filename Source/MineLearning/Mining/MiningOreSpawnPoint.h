#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MiningOreSpawnPoint.generated.h"

UCLASS()
class MINELEARNING_API AMiningOreSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AMiningOreSpawnPoint();

private:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* SceneRoot;
};
