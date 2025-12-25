// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "GameplayTagContainer.h"

#include "OWRPGInventoryFragment_Traits.generated.h"

/**
 * Fragment to define "Static Traits" for an item.
 * Use this to say "This item is Metal, Sharp, and Heavy".
 */
UCLASS(DisplayName = "Item Traits")
class OWRPGRUNTIME_API UOWRPGInventoryFragment_Traits : public ULyraInventoryItemFragment
{
	GENERATED_BODY()
	
public:
	// The main list of traits
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OWRPG")
	FGameplayTagContainer Traits;

	// Optional: A specific category tag for UI sorting (e.g., Weapon vs Food)
	// We keep this separate from Traits to make UI logic faster.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OWRPG", meta = (Categories = "OWRPG.Item.Category"))
	FGameplayTag ItemCategory;
};
