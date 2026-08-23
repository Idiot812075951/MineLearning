#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	IronOre UMETA(DisplayName = "Iron Ore"),
	Coin UMETA(DisplayName = "Coin"),
	Ammo UMETA(DisplayName = "Ammo")
};

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Ore UMETA(DisplayName = "Ore"),
	Currency UMETA(DisplayName = "Currency"),
	Ammo UMETA(DisplayName = "Ammo"),
	Weapon UMETA(DisplayName = "Weapon"),
	Consumable UMETA(DisplayName = "Consumable"),
	Misc UMETA(DisplayName = "Misc")
};

UENUM(BlueprintType)
enum class EItemReceiverType : uint8
{
	Processor UMETA(DisplayName = "Processor"),
	Warehouse UMETA(DisplayName = "Warehouse"),
	Gunner UMETA(DisplayName = "Gunner")
};

/**
 * Shared world-space presentation scales for loose and stored item meshes.
 * Keeping this in one place prevents the processor, carrier and warehouse
 * from each presenting the same item at a different size.
 */
namespace MineLearningItemVisual
{
	inline constexpr float GoldCoinScale = 3.3f;
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
