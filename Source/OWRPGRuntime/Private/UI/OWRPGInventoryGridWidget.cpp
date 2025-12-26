// Copyright Legion. All Rights Reserved.

#include "UI/OWRPGInventoryGridWidget.h"
#include "UI/OWRPGInventoryItemWidget.h"
#include "UI/OWRPGInventoryDragDrop.h"
#include "Inventory/OWRPGInventoryManagerComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"
#include "GameFramework/PlayerController.h" 
#include "InputCoreTypes.h"
#include <Components/SizeBox.h>

void UOWRPGInventoryGridWidget::InitializeGrid(UOWRPGInventoryManagerComponent* InManager)
{
	if (InventoryManager)
	{
		InventoryManager->OnInventoryRefresh.RemoveDynamic(this, &UOWRPGInventoryGridWidget::RefreshGrid);
	}

	InventoryManager = InManager;

	if (InventoryManager)
	{
		InventoryManager->OnInventoryRefresh.AddDynamic(this, &UOWRPGInventoryGridWidget::RefreshGrid);
		RefreshGrid();
	}
}

void UOWRPGInventoryGridWidget::NativeDestruct()
{
	Super::NativeDestruct();
	if (InventoryManager)
	{
		InventoryManager->OnInventoryRefresh.RemoveDynamic(this, &UOWRPGInventoryGridWidget::RefreshGrid);
	}
}

void UOWRPGInventoryGridWidget::RefreshGrid()
{
	if (!InventoryManager || !GridCanvas || !ItemWidgetClass) return;

	// 1. Calculate Required Pixel Size
	float TotalWidth = InventoryManager->Columns * TileSize;
	float TotalHeight = InventoryManager->Rows * TileSize;

	// 2. Force the Canvas to this size
	if (UCanvasPanelSlot* RootSlot = Cast<UCanvasPanelSlot>(GridCanvas->Slot))
	{
		RootSlot->SetSize(FVector2D(TotalWidth, TotalHeight));
	}

	// 3. Force the Widget itself to this size (Dynamic resizing)
	SetDesiredSizeInViewport(FVector2D(TotalWidth, TotalHeight));

	if (USizeBox* RootSize = Cast<USizeBox>(GetRootWidget())) {
	    RootSize->SetWidthOverride(TotalWidth);
	    RootSize->SetHeightOverride(TotalHeight);
	}

	GridCanvas->ClearChildren();
	ItemWidgets.Empty();

	for (const FOWRPGInventoryEntry& Entry : InventoryManager->InventoryList.Entries)
	{
		if (!Entry.Item) continue;

		UOWRPGInventoryItemWidget* NewWidget = CreateWidget<UOWRPGInventoryItemWidget>(this, ItemWidgetClass);
		NewWidget->ItemInstance = Entry.Item;
		NewWidget->InventoryManager = InventoryManager;
		NewWidget->TileSize = this->TileSize;
		NewWidget->bIsRotated = Entry.bRotated; // Visual Rotation
		NewWidget->UpdateVisual();

		UCanvasPanelSlot* CanvasSlot = GridCanvas->AddChildToCanvas(NewWidget);
		CanvasSlot->SetPosition(FVector2D(Entry.X * TileSize, Entry.Y * TileSize));

		int32 W, H;
		InventoryManager->GetItemDimensions(Entry.Item, W, H, Entry.bRotated);
		CanvasSlot->SetSize(FVector2D(W * TileSize, H * TileSize));

		ItemWidgets.Add(Entry.Item, NewWidget);
	}
}

// --- DRAG LOGIC ---

void UOWRPGInventoryGridWidget::CheckRotationInput()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		// Detect 'R' key press to toggle rotation
		// NOTE: This runs every frame during DragOver
		if (PC->WasInputKeyJustPressed(EKeys::R))
		{
			bIsDraggingRotated = !bIsDraggingRotated;
		}
	}
}

bool UOWRPGInventoryGridWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UOWRPGInventoryDragDrop* DragOp = Cast<UOWRPGInventoryDragDrop>(InOperation);
	if (!DragOp || !InventoryManager) return false;

	// 1. Rotation Input Check (If you implemented the 'R' key logic)
	CheckRotationInput();

	// 2. Determine Dimensions (Base + Rotation Flip)
	int32 BaseW, BaseH;
	if (DragOp->DraggedItemW > 0)
	{
		BaseW = DragOp->DraggedItemW;
		BaseH = DragOp->DraggedItemH;
	}
	else
	{
		// Fallback if not cached
		InventoryManager->GetItemDimensions(DragOp->DraggedItem, BaseW, BaseH, false);
	}

	// Apply Rotation Logic: If rotated, flip W and H
	bool bFinalRotated = (DragOp->bIsRotated != bIsDraggingRotated);

	if (bFinalRotated)
	{
		DraggedW = BaseH;
		DraggedH = BaseW;
	}
	else
	{
		DraggedW = BaseW;
		DraggedH = BaseH;
	}

	// 3. Calculate Grid Index based on Mouse Position
	FVector2D LocalPos = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());

	// Offset so we drag from the "Click Point" relative to the item, not always top-left
	LocalPos -= DragOp->DragOffset;

	int32 NewHoveredX = FMath::FloorToInt(LocalPos.X / TileSize);
	int32 NewHoveredY = FMath::FloorToInt(LocalPos.Y / TileSize);

	// 4. BOUNDARY CHECK (The Fix)
	// If the top-left corner of the drag is outside the grid, stop drawing immediately.
	if (NewHoveredX < 0 || NewHoveredY < 0 ||
		NewHoveredX >= InventoryManager->Columns ||
		NewHoveredY >= InventoryManager->Rows)
	{
		bIsDraggingOver = false; // Prevents NativePaint from drawing the box
		return false;            // Reject the drag over event
	}

	// 5. Update State
	HoveredX = NewHoveredX;
	HoveredY = NewHoveredY;

	// 6. Check Placement Validity (Logic for Red/Green)
	// Even if the cursor is valid (e.g. at 9,9), the item might be huge (2x2) and go out of bounds.
	bool bOutOfBounds = (HoveredX + DraggedW) > InventoryManager->Columns || (HoveredY + DraggedH) > InventoryManager->Rows;

	if (bOutOfBounds)
	{
		bIsPlacementValid = false; // Draw RED
	}
	else
	{
		// Check "Tetris" overlaps (Predictive)
		// We pass 'DragOp->DraggedItem' as the item to Exclude (ignore self)
		TArray<ULyraInventoryItemInstance*> Overlaps = InventoryManager->GetItemsInRect(HoveredX, HoveredY, DraggedW, DraggedH, DragOp->DraggedItem);

		// Logic: 0 overlaps = Green (Place). 1 overlap = Green (Swap). >1 overlaps = Red (Blocked).
		bIsPlacementValid = (Overlaps.Num() <= 1);
	}

	bIsDraggingOver = true; // Tells NativePaint to draw the box
	return true;
}

void UOWRPGInventoryGridWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	bIsDraggingOver = false;
	bIsDraggingRotated = false; // Reset on leave
}

bool UOWRPGInventoryGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UOWRPGInventoryDragDrop* DragOp = Cast<UOWRPGInventoryDragDrop>(InOperation);

	if (bIsPlacementValid && InventoryManager && DragOp)
	{
		// Calculate final rotation state
		bool bFinalRotated = (DragOp->bIsRotated != bIsDraggingRotated);

		InventoryManager->ServerAttemptMove(DragOp->DraggedItem, HoveredX, HoveredY, bFinalRotated);

		bIsDraggingOver = false;
		bIsDraggingRotated = false;
		return true;
	}

	return false;
}

int32 UOWRPGInventoryGridWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (bIsDraggingOver)
	{
		FLinearColor Color = bIsPlacementValid ? ValidDropColor : InvalidDropColor;

		FPaintGeometry PaintGeo = AllottedGeometry.ToPaintGeometry(
			FVector2D(DraggedW * TileSize, DraggedH * TileSize),
			FSlateLayoutTransform(FVector2D(HoveredX * TileSize, HoveredY * TileSize))
		);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			MaxLayer + 1,
			PaintGeo,
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			Color
		);
	}

	return MaxLayer;
}