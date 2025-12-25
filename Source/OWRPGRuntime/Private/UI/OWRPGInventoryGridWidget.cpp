#include "UI/OWRPGInventoryGridWidget.h"
#include "UI/OWRPGInventoryItemWidget.h"
#include "Inventory/OWRPGInventoryManagerComponent.h"
#include "UI/OWRPGInventoryDragDrop.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void UOWRPGInventoryGridWidget::InitializeGrid(UOWRPGInventoryManagerComponent* InManager)
{
	// Safety: Unbind if replacing existing manager
	if (InventoryManager)
	{
		InventoryManager->OnInventoryVisualsChanged.RemoveDynamic(this, &UOWRPGInventoryGridWidget::RefreshGrid);
	}

	InventoryManager = InManager;

	if (InventoryManager)
	{
		// Double check unbind to prevent duplicates
		InventoryManager->OnInventoryVisualsChanged.RemoveDynamic(this, &UOWRPGInventoryGridWidget::RefreshGrid);

		// Bind
		InventoryManager->OnInventoryVisualsChanged.AddDynamic(this, &UOWRPGInventoryGridWidget::RefreshGrid);

		RefreshGrid();
	}
}

void UOWRPGInventoryGridWidget::NativeDestruct()
{
	Super::NativeDestruct();
	if (InventoryManager)
	{
		InventoryManager->OnInventoryVisualsChanged.RemoveDynamic(this, &UOWRPGInventoryGridWidget::RefreshGrid);
	}
}

void UOWRPGInventoryGridWidget::RefreshGrid()
{
	if (!InventoryManager || !GridCanvas || !ItemWidgetClass) return;

	// 1. Resize Container
	if (GridSizeBox)
	{
		GridSizeBox->SetWidthOverride(InventoryManager->Columns * SlotSize);
		GridSizeBox->SetHeightOverride(InventoryManager->Rows * SlotSize);
	}

	// 2. Clear Old
	GridCanvas->ClearChildren();
	ActiveItemWidgets.Empty();

	// 3. Rebuild from "Diablo Array"
	const TArray<FOWRPGInventoryTile>& Tiles = InventoryManager->GridTiles;

	for (int32 i = 0; i < Tiles.Num(); i++)
	{
		const FOWRPGInventoryTile& Tile = Tiles[i];

		// Only spawn if this is the "Head" of the item
		if (Tile.Item != nullptr && Tile.bIsHead)
		{
			int32 Column = i % InventoryManager->Columns;
			int32 Row = i / InventoryManager->Columns;

			UOWRPGInventoryItemWidget* NewWidget = CreateWidget<UOWRPGInventoryItemWidget>(this, ItemWidgetClass);
			NewWidget->ItemInstance = Tile.Item;
			NewWidget->InventoryManager = InventoryManager;

			// PASS DYNAMIC CONFIG
			NewWidget->TileSize = this->SlotSize;
			NewWidget->UpdateVisual();

			UCanvasPanelSlot* CanvasSlot = GridCanvas->AddChildToCanvas(NewWidget);
			CanvasSlot->SetPosition(FVector2D(Column * SlotSize, Row * SlotSize));

			int32 W, H;
			InventoryManager->GetItemDimensions(Tile.Item, W, H);
			CanvasSlot->SetSize(FVector2D(W * SlotSize, H * SlotSize));

			ActiveItemWidgets.Add(Tile.Item, NewWidget);
		}
	}
}

bool UOWRPGInventoryGridWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UOWRPGInventoryDragDrop* DragOp = Cast<UOWRPGInventoryDragDrop>(InOperation);
	if (!DragOp || !InventoryManager) return false;

	// --- OPTIMIZATION: CACHE SIZE ---
	// If reset (1x1), try to fetch real size. DragOp usually has it if we set it in ItemWidget.
	if (CachedDraggedW == 1 && CachedDraggedH == 1)
	{
		// Preferred: Read from DragOp (no lookup needed)
		if (DragOp->DraggedItemW > 0)
		{
			CachedDraggedW = DragOp->DraggedItemW;
			CachedDraggedH = DragOp->DraggedItemH;
		}
		else
		{
			// Fallback: Query Manager
			InventoryManager->GetItemDimensions(DragOp->DraggedItem, CachedDraggedW, CachedDraggedH);
		}
	}

	// 1. Calculate Grid Hover Position
	FVector2D LocalPos = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	LocalPos -= DragOp->DragOffset;

	HoveredX = FMath::FloorToInt(LocalPos.X / SlotSize);
	HoveredY = FMath::FloorToInt(LocalPos.Y / SlotSize);

	// 2. Validate Placement
	if (HoveredX < 0 || HoveredY < 0 || (HoveredX + CachedDraggedW) > InventoryManager->Columns || (HoveredY + CachedDraggedH) > InventoryManager->Rows)
	{
		bIsPlacementValid = false;
	}
	else
	{
		// Collision Check (Preview)
		bool bCollision = false;
		for (int32 r = 0; r < CachedDraggedH; r++)
		{
			for (int32 c = 0; c < CachedDraggedW; c++)
			{
				int32 Index = InventoryManager->GetIndex(HoveredX + c, HoveredY + r);
				if (InventoryManager->GridTiles.IsValidIndex(Index))
				{
					if (InventoryManager->GridTiles[Index].Item != nullptr && InventoryManager->GridTiles[Index].Item != DragOp->DraggedItem)
					{
						// TODO: Check for valid Swap (1 item only) to allow Green light
						bCollision = true;
					}
				}
			}
		}
		bIsPlacementValid = !bCollision;
	}

	bIsDraggingOver = true;
	return true;
}

void UOWRPGInventoryGridWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	bIsDraggingOver = false;
	// Reset Cache on leave so next drag (which might be a different item) re-calculates
	CachedDraggedW = 1;
	CachedDraggedH = 1;
	HoveredX = -1;
	HoveredY = -1;
}

bool UOWRPGInventoryGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!bIsPlacementValid || !InventoryManager) return false;

	InventoryManager->ServerPlaceItem(HoveredX, HoveredY);

	bIsDraggingOver = false;
	return true;
}

int32 UOWRPGInventoryGridWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (bIsDraggingOver && InventoryManager)
	{
		FLinearColor Color = bIsPlacementValid ? FLinearColor(0.0f, 1.0f, 0.0f, 0.3f) : FLinearColor(1.0f, 0.0f, 0.0f, 0.3f);

		// Use CACHED values for pure math (no lookups)
		float DrawW = CachedDraggedW * SlotSize;
		float DrawH = CachedDraggedH * SlotSize;

		FPaintGeometry PaintGeo = AllottedGeometry.ToPaintGeometry(
			FVector2D(DrawW, DrawH),
			FSlateLayoutTransform(FVector2D(HoveredX * SlotSize, HoveredY * SlotSize))
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