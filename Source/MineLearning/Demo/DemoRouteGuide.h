#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DemoRouteGuide.generated.h"

class AMineLearningPlayerController;
class UInstancedStaticMeshComponent;

/** World-space presentation of the current objective's traversable navigation route. */
UCLASS()
class MINELEARNING_API ADemoRouteGuide : public AActor
{
	GENERATED_BODY()
public:
	ADemoRouteGuide();
	void Refresh(AMineLearningPlayerController* Controller);
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Segments;
	UPROPERTY(EditDefaultsOnly, Category="Guide") float HeightAboveGround = 4.f;
	UPROPERTY(EditDefaultsOnly, Category="Guide") float DotSpacing = 42.f;
	UPROPERTY(EditDefaultsOnly, Category="Guide") float DotDiameter = 9.f;
};
