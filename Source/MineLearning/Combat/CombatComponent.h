#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatConfig.h"
#include "CombatComponent.generated.h"

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCombatComponent();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat") TSoftObjectPtr<UCombatConfig> Config;
	UCombatConfig* GetConfig() const { return Config.LoadSynchronous(); }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat") FName PrimarySkillId = TEXT("Primary");
	UFUNCTION(BlueprintPure) FCombatAttributes GetAttributes() const;
	UFUNCTION(BlueprintPure) FCombatPanelViewData GetPanelData() const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly) void SetAttributeBonus(FCombatAttributes Bonus);
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnAttributesChanged;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
private:
	UPROPERTY(ReplicatedUsing = OnRep_Bonus) FCombatAttributes AttributeBonus;
	UFUNCTION() void OnRep_Bonus();
};
