#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatTypes.h"
#include "CombatDamageSubsystem.generated.h"

UCLASS()
class MINELEARNING_API UCombatDamageSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure) static bool CanDamageTarget(const AActor* Source, const AActor* Target);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly) FCombatDamageResult ApplyDamage(const FCombatDamageRequest& Request);
	// Explicit debug entry; shares validation and health settlement, never fabricates a skill.
	FCombatDamageResult ApplyDebugDamage(AActor* Target, float Amount);
};
