// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "OWRPGInventoryFragment_UI.generated.h"

class UTexture2D;

/**
 * Fragment to define how an item appears in the Menu/HUD.
 */
UCLASS(DisplayName = "UI Presentation")
class OWRPGRUNTIME_API UOWRPGInventoryFragment_UI : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UTexture2D> Icon;

	// Note: DO NOT add DisplayName here. We use the one in the parent ItemDefinition.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<UStaticMesh> WorldMesh;
};