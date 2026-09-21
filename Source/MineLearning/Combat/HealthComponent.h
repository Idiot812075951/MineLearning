#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "HealthComponent.generated.h"

UENUM(BlueprintType)
enum class ECombatFaction : uint8 { Player, Hostile, Resource };

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class MINELEARNING_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UHealthComponent();
	UFUNCTION(BlueprintPure) float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure) float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure) bool IsDead() const { return Health <= 0.f; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly) void Heal(float Amount);
	void InitializeHealth(float Maximum);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat") ECombatFaction Faction = ECombatFaction::Hostile;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat") bool bDestroyOnDeath = true;
	UPROPERTY(BlueprintAssignable) FCombatStateChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable) FCombatDamageResolved OnDamageResolved;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
	friend class UCombatDamageSubsystem;
	FCombatDamageResult ApplyResolvedDamage(const FCombatDamageRequest& Request, float Damage);
	UPROPERTY(ReplicatedUsing = OnRep_Health) float Health = 1000.f;
	UPROPERTY(ReplicatedUsing = OnRep_Health) float MaxHealth = 1000.f;
	UFUNCTION() void OnRep_Health();
};
