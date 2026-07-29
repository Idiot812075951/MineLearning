#include "MiningOreSpawnPoint.h"

#include "Components/SceneComponent.h"

AMiningOreSpawnPoint::AMiningOreSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}
