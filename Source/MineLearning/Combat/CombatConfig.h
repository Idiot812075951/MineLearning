#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatTypes.h"
#include "CombatConfig.generated.h"

/** Shared by damage execution and the skill/details UI. Intelligence is displayed as Energy. */
UCLASS(BlueprintType)
class MINELEARNING_API UCombatConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FCombatAttributes Attributes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1")) float MaxHealth = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FSkillDamageSpec> Skills;
	const FSkillDamageSpec* FindSkill(FName SkillId) const;
};
