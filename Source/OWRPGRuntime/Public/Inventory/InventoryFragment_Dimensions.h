// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "InventoryFragment_Dimensions.generated.h"

/**
 * Defines the size of an item in the inventory grid (Width x Height).
 */
UCLASS(DisplayName = "Inventory Dimensions")
class OWRPGRUNTIME_API UInventoryFragment_Dimensions : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:
	// Default to 1x1
	UInventoryFragment_Dimensions()
		: Width(1)
		, Height(1)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dimensions", meta = (ClampMin = 1))
	int32 Width;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dimensions", meta = (ClampMin = 1))
	int32 Height;
};