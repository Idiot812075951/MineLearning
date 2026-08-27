#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "ResourceCarryComponent.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class UStaticMesh;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCarryChangedSignature, int32, Current, int32, Max);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UResourceCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceCarryComponent();

	UFUNCTION(BlueprintPure, Category="Item|Carry")
	const FItemStack& GetCurrentItem() const { return CurrentItem; }

	UFUNCTION(BlueprintPure, Category="Item|Carry")
	int32 GetCurrentItemCount() const { return CurrentItem.Amount; }

	UFUNCTION(BlueprintPure, Category="Item|Carry")
	int32 GetCapacity() const { return Capacity; }

	UFUNCTION(BlueprintPure, Category="Item|Carry")
	bool IsEmpty() const { return !CurrentItem.IsValid(); }

	UFUNCTION(BlueprintPure, Category="Item|Carry")
	bool IsFull() const;

	UFUNCTION(BlueprintPure, Category="Item|Carry")
	bool CanAcceptItem(const FItemStack& Item) const;

	UFUNCTION(BlueprintCallable, Category="Item|Carry")
	int32 AddItem(const FItemStack& Item);

	int32 AddItemWithVisual(
		const FItemStack& Item,
		UStaticMesh* InResourceMesh,
		float InResourceMeshScale = 1.0f);

	UFUNCTION(BlueprintCallable, Category="Item|Carry")
	FItemStack TakeAllItems();

	UFUNCTION(BlueprintCallable, Category="Item|Carry")
	void ClearItems();

	/** Spawns one physical pickup per carried item and clears the carry only after every spawn succeeds. */
	UFUNCTION(BlueprintCallable, Category="Item|Carry")
	int32 DropAllItems(const FVector& DropOrigin);

	/** Sets capacity/category policy. Used by character constructors instead of class checks. */
	void ConfigureAcceptance(int32 InCapacity, bool bInAcceptAllCategories, const TArray<EItemCategory>& InAllowedCategories);

	// Compatibility API for the established mining/storage path. IronOre is the legacy ore type.
	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	int32 GetCurrentOreCount() const;

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	int32 GetMaxOreCount() const { return Capacity; }

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	bool CanAddOre(int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category="Mining|Carry")
	int32 AddOre(int32 Amount);

	int32 AddOreWithVisual(int32 Amount, UStaticMesh* InResourceMesh);

	UFUNCTION(BlueprintCallable, Category="Mining|Carry")
	int32 TakeAllOre();

	UFUNCTION(BlueprintCallable, Category="Mining|Carry")
	void ClearOre() { ClearItems(); }

	UPROPERTY(BlueprintAssignable, Category="Item|Carry")
	FOnCarryChangedSignature OnCarryChanged;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(VisibleAnywhere, Category="Item|Carry")
	FItemStack CurrentItem;

	UPROPERTY(EditAnywhere, Category="Item|Carry", meta=(ClampMin="1"))
	int32 Capacity = 5;

	UPROPERTY(EditAnywhere, Category="Item|Carry|Rules")
	bool bAcceptAllCategories = true;

	UPROPERTY(EditAnywhere, Category="Item|Carry|Rules", meta=(EditCondition="!bAcceptAllCategories"))
	TArray<EItemCategory> AllowedCategories;

	UPROPERTY(EditAnywhere, Category="Mining|Carry|Preview")
	UStaticMesh* PreviewResourceMesh = nullptr;

	UPROPERTY(EditAnywhere, Category="Mining|Carry|Preview")
	UMaterialInterface* PreviewResourceMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category="Mining|Carry|Preview")
	FName PreviewSocketName = TEXT("S_CargoBin");

	UPROPERTY(EditAnywhere, Category="Mining|Carry|Preview", meta=(MakeEditWidget=true))
	TArray<FTransform> PreviewResourceTransforms;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category="Mining|Carry|Preview")
	bool bShowFullPreviewInEditor = false;
#endif

	UPROPERTY(Transient)
	UInstancedStaticMeshComponent* PreviewResourcesMesh = nullptr;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PreviewResourceMaterialInstance = nullptr;

	UPROPERTY(Transient)
	UMaterialInterface* AppliedPreviewResourceMaterial = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CarriedResourceMesh = nullptr;

	UPROPERTY(Transient)
	float CarriedResourceMeshScale = 1.0f;

	void BroadcastCarryChanged();
	void RefreshPreviewResources();
	bool ConfigurePreviewResourcesMesh();
	UStaticMesh* GetPreviewResourceMesh() const;
	USkeletalMeshComponent* FindOwnerSkeletalMesh() const;
};
