// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "OWRPGInventoryDragDrop.generated.h"

class ULyraInventoryItemInstance;
class UOWRPGInventoryManagerComponent;
class UUserWidget;

/**
 * Payload for Inventory Drag & Drop operations.
 */
UCLASS()
class OWRPGRUNTIME_API UOWRPGInventoryDragDrop : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// --- WEAK REFERENCES (Prevents GC Crashes) ---

	/** The item being moved. WeakPtr ensures we don't keep the World alive if PIE stops. */
	TWeakObjectPtr<ULyraInventoryItemInstance> DraggedItem;

	/** Helper to get the item safely in Blueprints */
	UFUNCTION(BlueprintPure, Category = "Drag Drop")
	ULyraInventoryItemInstance* GetDraggedItem() const;

	/** Where the item came from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	TWeakObjectPtr<UOWRPGInventoryManagerComponent> SourceComponent;

	/** The widget that started the drag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	TWeakObjectPtr<UUserWidget> SourceWidget;

	// --- DATA ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	bool bRotated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag Drop")
	FVector2D DragOffset = FVector2D::ZeroVector;
};