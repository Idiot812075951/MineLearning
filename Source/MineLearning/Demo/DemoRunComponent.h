#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MineLearning/Combat/CombatTypes.h"
#include "MineLearning/PlayerTransformZone.h"
#include "MineLearning/Mining/ItemTypes.h"
#include "DemoRunComponent.generated.h"

class AWarehouseDepot;
class AMineableOre;
class APawn;
enum class EItemType : uint8;
struct FDemoGuidance;
struct FFormPurchaseRow;
class UAmmoInventoryComponent;
DECLARE_MULTICAST_DELEGATE_OneParam(FDemoUnitReady, APawn*);
DECLARE_DELEGATE_RetVal(bool, FPrepareProductionRun);
DECLARE_DELEGATE_RetVal_OneParam(bool, FFormPermissionQuery, EPlayerTransformationForm);

UENUM(BlueprintType)
enum class EDemoPhase : uint8 { Briefing, Production, BossChallenge, Victory };

UENUM(BlueprintType)
enum class EDemoCommand : uint8
{
	Start, ProcessFour, SellTwo, CancelOrders, BuyCarrier, BuyOreBuddy,
	UpgradeStrength, UpgradeAgility, UpgradeIntelligence, UnlockGunner, UnlockGuren,
	SubmitMaterials, Human, OreBuddy, Carrier, Gunner, Guren, Restart, ResetCalibration, BuyMagazine,
	UnlockAll
};

/** One local run; owned by the persistent player controller, independent of its pawn and UI. */
UCLASS(ClassGroup=(Demo), meta=(BlueprintSpawnableComponent))
class MINELEARNING_API UDemoRunComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDemoRunComponent();
	FDemoUnitReady OnUnitReady;
	FPrepareProductionRun PrepareRun;
	FFormPermissionQuery HasFormPermission;
	void ConfigureStartingCrew(int32 OreBuddies, int32 Carriers);
	void ConfigureFormCost(EPlayerTransformationForm Form, const TArray<FItemStack>& Cost);
	AWarehouseDepot* GetWarehouse() const { return Warehouse; }
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UFUNCTION(BlueprintPure, Category="Demo") bool IsEnabled() const;
	UFUNCTION(BlueprintPure, Category="Demo") EDemoPhase GetPhase() const { return Phase; }
	UFUNCTION(BlueprintCallable, Category="Demo") bool ExecuteCommand(EDemoCommand Command);
	UFUNCTION(BlueprintPure, Category="Demo") FText GetStatusText() const;
	UFUNCTION(BlueprintPure, Category="Demo") FText GetObjectiveText() const;
	UFUNCTION(BlueprintPure, Category="Demo") FText GetGuideText() const;
	UFUNCTION(BlueprintPure, Category="Demo") FText GetFeedbackText() const { return Feedback; }
	UFUNCTION(BlueprintPure, Category="Demo") FText GetResultText() const;
	UFUNCTION(BlueprintPure, Category="Demo") bool IsFinished() const;
	UFUNCTION(BlueprintPure, Category="Demo") bool IsRecommendedCommand(EDemoCommand Command) const;
	UFUNCTION(BlueprintPure, Category="Demo") bool IsTerminalRecommended() const;
	UFUNCTION(BlueprintPure, Category="Demo") FText GetNextActionText() const;
	bool GetGuidanceDestination(FVector& OutLocation) const;
	bool IsFormUnlocked(EPlayerTransformationForm Form) const;
	void ApplyCoreBonus(APawn* Pawn) const;
	UFUNCTION(BlueprintPure, Category="Demo") int32 GetReserveMagazines() const;
	UPROPERTY(BlueprintAssignable, Category="Demo") FCombatStateChanged OnRunChanged;
	UPROPERTY(EditAnywhere, Category="Demo|Boss", meta=(ClampMin="1")) float ChallengeDuration = 240.f;
	UPROPERTY(EditAnywhere, Category="Demo|Boss") FVector BossLocation = FVector(0.f, -850.f, 250.f);
	UPROPERTY(EditAnywhere, Category="Demo|Boss") TSoftClassPtr<AMineableOre> BossClass;
	UPROPERTY(EditAnywhere, Category="Demo|Boss", meta=(ClampMin="1")) int32 BossCoinCost = 20;
	UPROPERTY(EditAnywhere, Category="Demo|Boss", meta=(ClampMin="1")) int32 BossIngotCost = 12;
	UPROPERTY(EditAnywhere, Category="Demo") TSoftClassPtr<APawn> CarrierClass;
	UPROPERTY(EditAnywhere, Category="Demo") TSoftClassPtr<APawn> OreBuddyClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Demo") int32 PurchasedCarriers = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Demo") int32 PurchasedOreBuddies = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Demo") int32 BossAttempts = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Demo") float RemainingSeconds = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Demo") FCombatAttributes CoreBonus;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Demo") TObjectPtr<AMineableOre> BossTarget;
private:
	FDemoGuidance ResolveGuidance() const;
	void InitializeRun();
	void AdvanceClock();
	void Finish();
	bool BeginBossChallenge();
	void EndBossAttempt();
	bool PurchaseRobot(bool bCarrier);
	bool ChangeForm(EPlayerTransformationForm Form);
	bool Respond(bool bSuccess, const FText& Message);
	int32 Available(EItemType Item) const;
	UFUNCTION() void InventoryChanged();
	UFUNCTION() void TargetDamaged(const FCombatDamageRequest& Request, const FCombatDamageResult& Result);
	UFUNCTION() void BossDestroyed(AActor* Actor);
	UFUNCTION() void WorkerDestroyed(AActor* Worker);
	UPROPERTY(Transient) TObjectPtr<AWarehouseDepot> Warehouse;
	UPROPERTY(Transient) TArray<TObjectPtr<APawn>> Workers;
	EDemoPhase Phase = EDemoPhase::Briefing;
	FText Feedback;
	FTimerHandle ClockHandle;
	float StartedAt = 0.f;
	float BossStartedAt = 0.f;
	float FinishedAt = 0.f;
	TMap<EPlayerTransformationForm, TArray<FItemStack>> FormCosts;
	bool bGunnerUnlocked = false;
	bool bGurenUnlocked = false;
	int32 InitialOreBuddies = 0;
	int32 InitialCarriers = 0;
};
