// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "OWRPGInventoryFragment_CoreStats.generated.h"

/**
 * Fragment to define "Physical" stats for an item.
 * Pure logic: Weight, Stacking, Value.
 * NO VISUALS HERE.
 */
UCLASS(DisplayName = "Core Stats")
class OWRPGRUNTIME_API UOWRPGInventoryFragment_CoreStats : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:
	// Weight in kg per unit
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float Weight = 0.1f;

	// Maximum items in one stack
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1"))
	int32 MaxStack = 1;

	// Maximum items in one stack
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	int32 GoldValue = 0;
};