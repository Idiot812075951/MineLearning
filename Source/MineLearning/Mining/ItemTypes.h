#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

class UStaticMesh;

UENUM(BlueprintType)
enum class EItemType : uint8
{
	IronOre UMETA(DisplayName = "Iron Ore"),
	Coin UMETA(DisplayName = "Coin"),
	Ammo UMETA(DisplayName = "Ammo"),
	IronIngot UMETA(DisplayName = "Iron Ingot")
};

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Ore UMETA(DisplayName = "Ore"),
	Currency UMETA(DisplayName = "Currency"),
	Ammo UMETA(DisplayName = "Ammo"),
	Weapon UMETA(DisplayName = "Weapon"),
	Consumable UMETA(DisplayName = "Consumable"),
	Misc UMETA(DisplayName = "Misc"),
	ProcessedMaterial UMETA(DisplayName = "Processed Material")
};

UENUM(BlueprintType)
enum class EItemReceiverType : uint8
{
	Processor UMETA(DisplayName = "Processor"),
	Warehouse UMETA(DisplayName = "Warehouse"),
	SellPoint UMETA(DisplayName = "Sell Point"),
	Gunner UMETA(DisplayName = "Gunner")
};

/**
 * Shared world-space presentation scales for loose and stored item meshes.
 * Keeping this in one place prevents the processor, carrier and warehouse
 * from each presenting the same item at a different size.
 */
namespace MineLearningItemVisual
{
	inline constexpr float StandardMaxDimensionCm = 30.0f;
	inline constexpr int32 DefaultStackHeight = 3;

	MINELEARNING_API float GetUniformScale(const UStaticMesh* Mesh);
	MINELEARNING_API FVector GetRelativeScale(
		const UStaticMesh* Mesh,
		const FVector& ParentWorldScale);
	MINELEARNING_API FVector GetWorldSize(const UStaticMesh* Mesh);
}

USTRUCT(BlueprintType)
struct MINELEARNING_API FItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	EItemType ItemType = EItemType::IronOre;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item", meta=(ClampMin="0"))
	int32 Amount = 0;

	bool IsValid() const
	{
		return Amount > 0;
	}
};
