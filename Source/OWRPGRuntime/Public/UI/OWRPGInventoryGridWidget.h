// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OWRPGInventoryGridWidget.generated.h"

class UOWRPGInventoryManagerComponent;
class UOWRPGInventoryItemWidget;
class UCanvasPanel;
class USizeBox;

/**
 * The Grid Container. Handles Drag Over logic and Painting the Highlight.
 */
UCLASS()
class OWRPGRUNTIME_API UOWRPGInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- CONFIG ---
	UPROPERTY(BlueprintReadWrite, Category = "Inventory", meta = (ExposeOnSpawn = true))
	TObjectPtr<UOWRPGInventoryManagerComponent> InventoryManager;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UOWRPGInventoryItemWidget> ItemWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float SlotSize = 50.0f;

protected:
	// --- WIDGET BINDINGS ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> GridCanvas;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> GridSizeBox;

	// --- STATE ---
	UPROPERTY()
	TMap<TObjectPtr<class ULyraInventoryItemInstance>, TObjectPtr<UOWRPGInventoryItemWidget>> ActiveItemWidgets;

	// --- DRAG HIGHLIGHT STATE ---
	bool bIsDraggingOver = false;
	bool bIsPlacementValid = false;

	// Where is the mouse hovering? (Grid Coords)
	int32 HoveredX = -1;
	int32 HoveredY = -1;

	// Cache the size of the item being dragged (so we don't query Manager every frame in OnPaint)
	int32 CachedDraggedW = 1;
	int32 CachedDraggedH = 1;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeGrid(UOWRPGInventoryManagerComponent* InManager);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshGrid();

protected:
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};