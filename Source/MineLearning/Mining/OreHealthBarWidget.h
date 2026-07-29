#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OreHealthBarWidget.generated.h"

class AMineableOre;
class UProgressBar;
class UTextBlock;

UCLASS()
class MINELEARNING_API UOreHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetObservedOre(AMineableOre* InOre);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleOreHealthChanged(float CurrentHealth, float MaxHealth);

	void RefreshFromOre();

	UPROPERTY(Transient)
	AMineableOre* ObservedOre = nullptr;

	UPROPERTY(Transient)
	UProgressBar* HealthProgressBar = nullptr;

	UPROPERTY(Transient)
	UTextBlock* HealthText = nullptr;
};
