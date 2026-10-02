#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponAppearanceComponent.generated.h"

class UCombatComponent;
class UMeshComponent;
class UMaterialInterface;

USTRUCT()
struct FWeaponMaterialSlot
{
	GENERATED_BODY()
	UPROPERTY() TObjectPtr<UMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> Original;
	int32 Index = 0;
};

/** Presentation subscribes to combat style state; combat never calls this component. */
UCLASS(ClassGroup = (Presentation), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UWeaponAppearanceComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWeaponAppearanceComponent();
	UPROPERTY(EditAnywhere, Category = "Appearance")
	TArray<FName> MeshNames = {TEXT("WeaponMesh"), TEXT("MagazineMesh")};
	UPROPERTY(EditAnywhere, Category="Appearance") TMap<FName, TSoftObjectPtr<UMaterialInterface>> WeaponMaterials;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UFUNCTION() void RefreshAppearance();
	UPROPERTY(Transient) TObjectPtr<UCombatComponent> Combat;
	UPROPERTY(Transient) TArray<FWeaponMaterialSlot> Slots;
	FName AppliedStyle;
};
