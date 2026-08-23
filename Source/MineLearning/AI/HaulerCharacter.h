#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HaulerCharacter.generated.h"

class UResourceCarryComponent;
class UAnimSequence;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class MINELEARNING_API AHaulerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHaulerCharacter();
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="Item|Hauler")
	UResourceCarryComponent* GetResourceCarryComponent() const { return ResourceCarryComponent; }

	void ShowCarriedItem(UStaticMesh* ItemMesh);
	void HideCarriedItem();

	void PlayPickupAnimation();
	void PlayDropOffAnimation();

	UFUNCTION(BlueprintCallable, Category="Item|Hauler|Animation")
	void HandlePickupNotify();

	UFUNCTION(BlueprintCallable, Category="Item|Hauler|Animation")
	void HandleDropOffNotify();

	UFUNCTION(BlueprintPure, Category="Item|Hauler|Animation")
	bool HasVisibleCargo() const;

protected:
	void PlayInteractionAnimation(UAnimSequence* Sequence, bool bPickup);
	void HandlePickupAnimationFinished();
	void HandleDropOffAnimationFinished();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item|Hauler")
	TObjectPtr<UResourceCarryComponent> ResourceCarryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item|Hauler")
	TObjectPtr<UStaticMeshComponent> CarriedItemVisual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item|Hauler")
	TObjectPtr<UStaticMeshComponent> CargoContentVisual;

	UPROPERTY(EditDefaultsOnly, Category="Item|Hauler|Animation")
	TObjectPtr<UAnimSequence> PickupAnimation;

	UPROPERTY(EditDefaultsOnly, Category="Item|Hauler|Animation")
	TObjectPtr<UAnimSequence> DropOffAnimation;

	FTimerHandle InteractionNotifyFallbackHandle;
	FTimerHandle InteractionFinishedHandle;
};
