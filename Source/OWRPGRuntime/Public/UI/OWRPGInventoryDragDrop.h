// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "OWRPGInventoryDragDrop.generated.h"

class ULyraInventoryItemInstance;

/**
 * Custom Drag Drop Operation to carry Item Data + Cached Dimensions + Offset
 */
UCLASS()
class OWRPGRUNTIME_API UOWRPGInventoryDragDrop : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// The item instance being dragged
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	TObjectPtr<ULyraInventoryItemInstance> DraggedItem;

	// The source widget that started the drag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	TObjectPtr<UUserWidget> SourceWidget;

	// --- NEW: Cached Dimensions (Fixes the C2039 Error) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	int32 DraggedItemW = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	int32 DraggedItemH = 1;

	// --- NEW: Mouse Offset (Fixes the Drag Alignment) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	FVector2D DragOffset = FVector2D::ZeroVector;
};