#pragma once

#include "CoreMinimal.h"
#include "MiningTypes.generated.h"

UENUM(BlueprintType)
enum class EResourceType : uint8
{
	Stone UMETA(DisplayName = "Stone"),
	Iron  UMETA(DisplayName = "Iron"),
	Gold  UMETA(DisplayName = "Gold")
};

UENUM(BlueprintType)
enum class EOreDropTrigger : uint8
{
	OnMiningHit UMETA(DisplayName = "On Mining Hit"),
	OnDepleted UMETA(DisplayName = "On Depleted")
};

USTRUCT(BlueprintType)
struct FMiningHitRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MiningPower = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ToolEfficiency = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* InstigatorActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HitNormal = FVector::UpVector;
};

USTRUCT(BlueprintType)
struct FOreDropRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EResourceType ResourceType = EResourceType::Stone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOreDropTrigger Trigger = EOreDropTrigger::OnMiningHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DropChance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	int32 MinAmount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	int32 MaxAmount = 1;
};
