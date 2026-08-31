#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerTransformZone.generated.h"

class APlayerController;
class APawn;
class UBoxComponent;

UENUM(BlueprintType)
enum class EPlayerTransformationForm : uint8
{
	Human,
	OreBuddy,
	Gunner
};

/** Transformation area that owns the shared human/robot pawn-swap transaction. */
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

	UFUNCTION(BlueprintCallable, Category="Transformation")
	bool TrySelectForm(APlayerController* PlayerController, EPlayerTransformationForm Form);

	/** Finds an overlapping transformation area and enters its configured robot form. */
	UFUNCTION(BlueprintCallable, Category="Transformation")
	static bool TryTransformOverlappingPlayer(APlayerController* PlayerController);

	/** Finds an overlapping transformation area and restores the configured human form. */
	UFUNCTION(BlueprintCallable, Category="Transformation")
	static bool TryRestoreOverlappingPlayer(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category="Transformation")
	static bool TrySelectOverlappingForm(APlayerController* PlayerController, EPlayerTransformationForm Form);

	UFUNCTION(BlueprintPure, Category="Transformation")
	static bool IsPlayerOverlappingTransformZone(APlayerController* PlayerController);

private:
	UPROPERTY(VisibleAnywhere, Category="Transformation")
	TObjectPtr<UBoxComponent> TransformArea;

	UPROPERTY(EditAnywhere, Category="Transformation")
	TSubclassOf<APawn> RobotPawnClass;

	UPROPERTY(EditAnywhere, Category="Transformation")
	TSubclassOf<APawn> OreBuddyPawnClass;

	UPROPERTY(EditAnywhere, Category="Transformation")
	TSubclassOf<APawn> HumanPawnClass;

	static APlayerTransformZone* FindOverlappingZone(APlayerController* PlayerController);
	bool TrySwapPawn(APlayerController* PlayerController, TSubclassOf<APawn> TargetPawnClass);
};
