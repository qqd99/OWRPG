// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "OWRPGInventoryFragment_Pickup.generated.h"

class AOWRPGWorldCollectable;

/**
 * Fragment to define what actor to spawn when this item is dropped.
 */
UCLASS(DisplayName = "Pickup Definition")
class OWRPGRUNTIME_API UOWRPGInventoryFragment_Pickup : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:
	// The World Collectable actor to spawn (e.g., B_Pickup_Sword)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop")
	TSubclassOf<AOWRPGWorldCollectable> PickupActorClass;
};