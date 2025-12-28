// Copyright Legion. All Rights Reserved.

#include "UI/OWRPGInventoryGridWidget.h"
#include "UI/OWRPGInventoryItemWidget.h"
#include "UI/OWRPGInventoryDragDrop.h"
#include "Inventory/OWRPGInventoryManagerComponent.h"
#include "Inventory/OWRPGInventoryFunctionLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UOWRPGInventoryGridWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (DragHighlight)
	{
		DragHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// Stop the Memory Leak / GC Error
void UOWRPGInventoryGridWidget::NativeDestruct()
{
	Super::NativeDestruct();

	if (InventoryManager)
	{
		InventoryManager->OnInventoryRefresh.RemoveDynamic(this, &UOWRPGInventoryGridWidget::RefreshGrid);
	}

	// FIX: Force clear all widgets so they release references immediately
	WidgetPool.Empty();
	ActiveItemWidgets.Empty();
	if (GridCanvas)
	{
		GridCanvas->ClearChildren();
	}
}

void UOWRPGInventoryGridWidget::InitializeGrid(UOWRPGInventoryManagerComponent* InManager)
{
	if (!InManager) return;

	// Unbind potential old bindings first to be safe
	if (InventoryManager)
	{
		InventoryManager->OnInventoryRefresh.RemoveDynamic(this, &UOWRPGInventoryGridWidget::RefreshGrid);
	}

	InventoryManager = InManager;

	// Dynamic Resizing logic
	// This ensures the grid visual always matches the data (10x10, 5x5, etc)
	if (GridSizeBox)
	{
		float TotalWidth = InManager->Columns * TileSize;
		float TotalHeight = InManager->Rows * TileSize;

		GridSizeBox->SetWidthOverride(TotalWidth);
		GridSizeBox->SetHeightOverride(TotalHeight);
	}

	// Bind to updates
	InventoryManager->OnInventoryRefresh.AddDynamic(this, &UOWRPGInventoryGridWidget::RefreshGrid);

	// Initial Draw
	DrawGridLines();
	RefreshGrid();
}

void UOWRPGInventoryGridWidget::DrawGridLines()
{
	if (!BackgroundCanvas || !InventoryManager) return;
	// TODO Optional: You can implement dynamic line drawing here using OnPaint 
	// or by spawning Image widgets into BackgroundCanvas if strictly necessary.
}

// ==============================================================================
// POOLING REFRESH (The Optimization)
// ==============================================================================
void UOWRPGInventoryGridWidget::RefreshGrid()
{
	if (!InventoryManager || !GridCanvas || !ItemWidgetClass) return;

	// 1. Mark all active widgets as potentially unused
	TSet<ULyraInventoryItemInstance*> ProcessedItems;

	// We use the raw list to know "What exists"
	const TArray<FOWRPGInventoryEntry>& Entries = InventoryManager->InventoryList.Entries;

	for (const FOWRPGInventoryEntry& Entry : Entries)
	{
		if (!Entry.Item) continue;

		ProcessedItems.Add(Entry.Item);

		UOWRPGInventoryItemWidget* Widget = nullptr;

		// Check if we already have a widget for this item
		if (TObjectPtr<UOWRPGInventoryItemWidget>* FoundWidgetPtr = ActiveItemWidgets.Find(Entry.Item))
		{
			Widget = *FoundWidgetPtr;
		}
		else
		{
			// Create/Pool new widget
			Widget = GetFreeWidget();
			ActiveItemWidgets.Add(Entry.Item, Widget);
			GridCanvas->AddChild(Widget);
		}

		// Update Position
		if (Widget)
		{
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
			{
				CanvasSlot->SetPosition(FVector2D(Entry.X * TileSize, Entry.Y * TileSize));

				// Update Size
				int32 W, H;
				InventoryManager->GetItemDimensions(Entry.Item, W, H, Entry.bRotated);
				CanvasSlot->SetSize(FVector2D(W * TileSize, H * TileSize));
			}

			// Refresh Data
			Widget->Init(Entry.Item, InventoryManager, TileSize, false);
			Widget->SetVisibility(ESlateVisibility::Visible);
		}
	}

	// Cleanup Unused Widgets
	TArray<ULyraInventoryItemInstance*> ItemsToRemove;
	for (auto& Elem : ActiveItemWidgets)
	{
		// Elem.Key is TObjectPtr, implicit cast to Raw Pointer for Set lookup is fine usually, 
		// but explicit Get() is safer if strict.
		if (!ProcessedItems.Contains(Elem.Key.Get()))
		{
			ItemsToRemove.Add(Elem.Key.Get());
			// Return widget to pool
			if (Elem.Value)
			{
				Elem.Value->SetVisibility(ESlateVisibility::Collapsed);
				WidgetPool.Add(Elem.Value);
			}
		}
	}

	for (ULyraInventoryItemInstance* Item : ItemsToRemove)
	{
		ActiveItemWidgets.Remove(Item);
	}
}

UOWRPGInventoryItemWidget* UOWRPGInventoryGridWidget::GetFreeWidget()
{
	if (WidgetPool.Num() > 0)
	{
		// Pop returns TObjectPtr, implicit cast to raw pointer
		return WidgetPool.Pop();
	}

	// Create new
	return CreateWidget<UOWRPGInventoryItemWidget>(this, ItemWidgetClass);
}

// ==============================================================================
// DRAG AND DROP
// ==============================================================================

bool UOWRPGInventoryGridWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UOWRPGInventoryDragDrop* DragOp = Cast<UOWRPGInventoryDragDrop>(InOperation);
	if (!DragOp || !InventoryManager) return false;

	// Check if Source is still valid (Weak Ptr)
	if (!DragOp->SourceComponent.IsValid()) return false;

	// 1. Get Mouse Position in Local Grid Space
	FVector2D MousePos = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());

	// 2. Calculate Top-Left based on DragOffset (Half Size)
	// Example: Mouse is at (100, 100). Item is 100x100. Offset is (50, 50).
	// Top Left = (50, 50). Slot = 1,1.
	FVector2D ItemTopLeft = MousePos - DragOp->DragOffset;

	int32 HoveredX = FMath::RoundToInt(ItemTopLeft.X / TileSize);
	int32 HoveredY = FMath::RoundToInt(ItemTopLeft.Y / TileSize);

	int32 W, H;
	InventoryManager->GetItemDimensions(DragOp->DraggedItem.Get(), W, H, DragOp->bRotated);

	// Check Bounds
	bool bOutOfBounds = (HoveredX < 0 || HoveredY < 0 || (HoveredX + W) > InventoryManager->Columns || (HoveredY + H) > InventoryManager->Rows);

	if (bOutOfBounds)
	{
		if (DragHighlight) DragHighlight->SetVisibility(ESlateVisibility::Collapsed);
		return false;
	}

	// UPDATE HIGHLIGHT
	if (DragHighlight)
	{
		if (UCanvasPanelSlot* HighlightSlot = Cast<UCanvasPanelSlot>(DragHighlight->Slot))
		{
			HighlightSlot->SetPosition(FVector2D(HoveredX * TileSize, HoveredY * TileSize));
			HighlightSlot->SetSize(FVector2D(W * TileSize, H * TileSize));
		}
		DragHighlight->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	return true;
}

bool UOWRPGInventoryGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (DragHighlight) DragHighlight->SetVisibility(ESlateVisibility::Collapsed);

	UOWRPGInventoryDragDrop* DragOp = Cast<UOWRPGInventoryDragDrop>(InOperation);
	if (!DragOp || !InventoryManager) return false;

	// 1. Logic
	if (DragOp->SourceComponent.IsValid()) // Check WeakPtr
	{
		// Unhide source
		if (DragOp->SourceWidget.IsValid())
		{
			DragOp->SourceWidget->SetVisibility(ESlateVisibility::Visible);
		}

		FVector2D MousePos = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
		FVector2D ItemTopLeft = MousePos - DragOp->DragOffset;

		int32 DestX = FMath::RoundToInt(ItemTopLeft.X / TileSize);
		int32 DestY = FMath::RoundToInt(ItemTopLeft.Y / TileSize);

		InventoryManager->ServerTransferItem(
			DragOp->SourceComponent.Get(),
			DragOp->DraggedItem.Get(),
			DestX,
			DestY,
			DragOp->bRotated
		);
	}

	// 2. Manually clear references to prevent Memory Leaks / GC Crash
	// The Editor Engine caches the DragOp, so we must strip it of all World references.
	DragOp->DraggedItem = nullptr;
	DragOp->SourceComponent.Reset();
	DragOp->SourceWidget.Reset();
	DragOp->DefaultDragVisual = nullptr; // Breaks link to the Widget Tree

	return true;
}

void UOWRPGInventoryGridWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	// Hide Highlight
	if (DragHighlight) DragHighlight->SetVisibility(ESlateVisibility::Collapsed);
}