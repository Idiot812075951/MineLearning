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
enum class EMineableDamageStage : uint8
{
	Full,
	LightDamage,
	MediumDamage,
	HeavyDamage,
	Destroyed
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
struct FResourceDropConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EResourceType ResourceType = EResourceType::Stone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AmountPerDrop = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DropChance = 0.35f;
};