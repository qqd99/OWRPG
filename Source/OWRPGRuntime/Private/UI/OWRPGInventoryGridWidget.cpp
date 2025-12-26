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

	if (UCanvasPanelSlot* RootSlot = Cast<UCanvasPanelSlot>(GridCanvas->Slot))
	{
		RootSlot->SetSize(FVector2D(InventoryManager->Columns * TileSize, InventoryManager->Rows * TileSize));
	}
	SetDesiredSizeInViewport(FVector2D(InventoryManager->Columns * TileSize, InventoryManager->Rows * TileSize));

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

	// 1. Input Check
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
		InventoryManager->GetItemDimensions(DragOp->DraggedItem, BaseW, BaseH, false);
	}

	// Apply Rotation Logic: If rotated, flip W and H
	// We combine the Item's original rotation state with our toggle
	bool bFinalRotated = (DragOp->bIsRotated != bIsDraggingRotated); // XOR logic: Flip if 'R' pressed

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

	// 3. Grid Position
	FVector2D LocalPos = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	LocalPos -= DragOp->DragOffset;

	HoveredX = FMath::FloorToInt(LocalPos.X / TileSize);
	HoveredY = FMath::FloorToInt(LocalPos.Y / TileSize);

	// 4. Validate
	if (HoveredX < 0 || HoveredY < 0 || (HoveredX + DraggedW) > InventoryManager->Columns || (HoveredY + DraggedH) > InventoryManager->Rows)
	{
		bIsPlacementValid = false;
	}
	else
	{
		// "Tetris" Predictive Check
		TArray<ULyraInventoryItemInstance*> Overlaps = InventoryManager->GetItemsInRect(HoveredX, HoveredY, DraggedW, DraggedH, DragOp->DraggedItem);
		bIsPlacementValid = (Overlaps.Num() <= 1); // 0 = Place, 1 = Swap, >1 = Fail
	}

	bIsDraggingOver = true;
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