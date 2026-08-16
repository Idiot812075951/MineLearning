#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	int32 GetCurrentOreCount() const { return CurrentOreCount; }

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	int32 GetMaxOreCount() const { return MaxOreCount; }

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	bool IsFull() const;

	UFUNCTION(BlueprintPure, Category="Mining|Carry")
	bool CanAddOre(int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category="Mining|Carry")
	int32 AddOre(int32 Amount);

	/** Adds ore and, when the cargo bin was empty, uses the pickup's selected visual for its preview instances. */
	int32 AddOreWithVisual(int32 Amount, UStaticMesh* InResourceMesh);

	UFUNCTION(BlueprintCallable, Category="Mining|Carry")
	int32 TakeAllOre();

	UFUNCTION(BlueprintCallable, Category="Mining|Carry")
	void ClearOre();

	UPROPERTY(BlueprintAssignable, Category="Mining|Carry")
	FOnCarryChangedSignature OnCarryChanged;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(VisibleAnywhere, Category="Mining|Carry")
	int32 CurrentOreCount = 0;

	UPROPERTY(EditAnywhere, Category="Mining|Carry")
	int32 MaxOreCount = 5;

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

	/** The visual selected by the first pickup in the current cargo load. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CarriedResourceMesh = nullptr;

	void BroadcastCarryChanged();
	void RefreshPreviewResources();
	bool ConfigurePreviewResourcesMesh();
	UStaticMesh* GetPreviewResourceMesh() const;
	USkeletalMeshComponent* FindOwnerSkeletalMesh() const;
};
