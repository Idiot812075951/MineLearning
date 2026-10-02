#pragma once

#include "CoreMinimal.h"
#include "RunContentView.generated.h"

class UMaterialInterface;

/** Shared read-only content contract for draft cards, codex cards and talent/identity nodes. */
USTRUCT(BlueprintType)
struct FRunContentView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FName Id;
	UPROPERTY(BlueprintReadOnly) FText Title;
	UPROPERTY(BlueprintReadOnly) FText Summary;
	UPROPERTY(BlueprintReadOnly) FText Details;
	UPROPERTY(BlueprintReadOnly) FText Category;
	UPROPERTY(BlueprintReadOnly) FText State;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UMaterialInterface> Icon;
	UPROPERTY(BlueprintReadOnly) bool bEnabled = false;
	UPROPERTY(BlueprintReadOnly) FVector2D Position = FVector2D::ZeroVector;
	UPROPERTY(BlueprintReadOnly) FText UnitLabel;
};

USTRUCT(BlueprintType)
struct FTalentLinkView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FVector2D Start = FVector2D::ZeroVector;
	UPROPERTY(BlueprintReadOnly) FVector2D End = FVector2D::ZeroVector;
};
