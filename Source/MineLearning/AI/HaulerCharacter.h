#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AutonomousUnit.h"
#include "HaulerCharacter.generated.h"

class UResourceCarryComponent;
class UAnimSequence;
class UStaticMesh;
class UStaticMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class USceneComponent;
struct FInputActionValue;

UCLASS(Blueprintable)
class MINELEARNING_API AHaulerCharacter : public ACharacter, public IAutonomousUnit
{
	GENERATED_BODY()

public:
	AHaulerCharacter();
	virtual bool SupportsAutonomousControl() const override { return AIControllerClass != nullptr; }
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	UFUNCTION(BlueprintCallable, Category="Item|Hauler") bool TryPlayerTransfer();
	FText GetPlayerTransferFailureReason() const;
	UFUNCTION(BlueprintPure, Category="Item|Hauler") FText GetPlayerCargoDescription() const;
	bool GetPlayerDeliveryLocation(FVector& OutLocation) const;
	float GetInteractionRange() const;

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
	void MovePlayer(const FInputActionValue& Value);
	void TransferPlayer();
	void LookPlayer(const FInputActionValue& Value);
	UPROPERTY(VisibleAnywhere, Category="Player Control") TObjectPtr<USpringArmComponent> PlayerCameraBoom;
	UPROPERTY(VisibleAnywhere, Category="Player Control") TObjectPtr<UCameraComponent> PlayerCamera;
	UPROPERTY() TObjectPtr<UInputMappingContext> PlayerMapping;
	UPROPERTY() TObjectPtr<UInputAction> PlayerMoveAction;
	UPROPERTY() TObjectPtr<UInputAction> PlayerLookAction;
	UPROPERTY(Transient) TObjectPtr<AActor> PlayerDeliveryActor;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> PlayerDeliveryPoint;
	float NextPlayerTransferTime = 0.f;
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
