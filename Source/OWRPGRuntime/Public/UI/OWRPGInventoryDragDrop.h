// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "OWRPGInventoryDragDrop.generated.h"

class ULyraInventoryItemInstance;

/**
 * Payload for dragging an item.
 * Stores the item data and cached dimensions for the Grid to read efficiently.
 */
UCLASS()
class OWRPGRUNTIME_API UOWRPGInventoryDragDrop : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// The data being dragged
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	TObjectPtr<ULyraInventoryItemInstance> DraggedItem;

	// The widget that started the drag (useful if we need to show it again on cancel)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	TObjectPtr<UUserWidget> SourceWidget;

	// --- CACHED PROPERTIES (Optimization) ---

	// Width in grid cells
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	int32 DraggedItemW = 1;

	// Height in grid cells
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	int32 DraggedItemH = 1;

	// Is the dragged item currently rotated?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	bool bIsRotated = false;

	// Mouse offset from the top-left of the item widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	FVector2D DragOffset = FVector2D::ZeroVector;
};