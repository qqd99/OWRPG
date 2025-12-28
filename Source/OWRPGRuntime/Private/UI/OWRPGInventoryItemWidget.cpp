// Copyright Legion. All Rights Reserved.

#include "UI/OWRPGInventoryItemWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h" 
#include "Inventory/OWRPGInventoryFragment_UI.h"
#include "Inventory/OWRPGInventoryFragment_CoreStats.h"
#include "Inventory/OWRPGInventoryFunctionLibrary.h" 
#include "Inventory/OWRPGInventoryManagerComponent.h"
#include "UI/OWRPGInventoryDragDrop.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "System/OWRPGGameplayTags.h"

void UOWRPGInventoryItemWidget::NativeDestruct()
{
	Super::NativeDestruct();
	// Stop reference cycles
	SetToolTip(nullptr);
	ItemInstance.Reset();
	InventoryManager.Reset();
}

void UOWRPGInventoryItemWidget::Init(ULyraInventoryItemInstance* InItem, UOWRPGInventoryManagerComponent* InManager, float InTileSize, bool bIsDragVisual)
{
	InventoryManager = InManager;
	TileSize = InTileSize;
	bIsDragVisualWidget = bIsDragVisual; // Store Flag

	Refresh(InItem);
}

void UOWRPGInventoryItemWidget::Refresh(ULyraInventoryItemInstance* InItem)
{
	ItemInstance = InItem;

	if (!InItem)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	SetVisibility(ESlateVisibility::Visible);

	if (IconImage)
	{
		UTexture2D* IconTex = UOWRPGInventoryFunctionLibrary::GetItemIcon(InItem);
		if (IconTex)
		{
			IconImage->SetBrushFromTexture(IconTex);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetBrushFromTexture(nullptr);
		}
	}

	if (StackCountText)
	{
		int32 Count = UOWRPGInventoryFunctionLibrary::GetItemStatsStackCount(InItem);
		if (Count > 1)
		{
			StackCountText->SetText(FText::AsNumber(Count));
			StackCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			StackCountText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// CRITICAL FIX: If this is a drag visual, NEVER create a tooltip.
	// This breaks the reference chain that keeps the World alive in the Editor.
	if (bIsDragVisualWidget)
	{
		SetToolTip(nullptr);
		// Set HitTestInvisible so Slate doesn't try to interact with it
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (!IsDesignTime())
	{
		FText Name = UOWRPGInventoryFunctionLibrary::GetItemDisplayName(InItem);
		if (!Name.IsEmpty())
		{
			SetToolTipText(Name);
		}
	}
}

void UOWRPGInventoryItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	if (ULyraInventoryItemInstance* Item = Cast<ULyraInventoryItemInstance>(ListItemObject))
	{
		Refresh(Item);
	}
}

FReply UOWRPGInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void UOWRPGInventoryItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!ItemInstance.IsValid() || !InventoryManager.IsValid()) return;

	UOWRPGInventoryDragDrop* DragOp = NewObject<UOWRPGInventoryDragDrop>();

	// Weak Pointers
	DragOp->DraggedItem = ItemInstance.Get();
	DragOp->SourceComponent = InventoryManager.Get();
	DragOp->SourceWidget = this;

	int32 W, H;
	InventoryManager->GetItemDimensions(ItemInstance.Get(), W, H, false);
	float WidthPX = W * TileSize;
	float HeightPX = H * TileSize;

	// Center Snap Offset
	DragOp->DragOffset = FVector2D(WidthPX * 0.5f, HeightPX * 0.5f);

	USizeBox* VisualContainer = NewObject<USizeBox>(this);
	VisualContainer->SetWidthOverride(WidthPX);
	VisualContainer->SetHeightOverride(HeightPX);

	// Create Visual with Flag = true
	UOWRPGInventoryItemWidget* VisualWidget = CreateWidget<UOWRPGInventoryItemWidget>(this, GetClass());
	VisualWidget->Init(ItemInstance.Get(), InventoryManager.Get(), TileSize, true); // <--- TRUE

	VisualContainer->SetContent(VisualWidget);

	DragOp->DefaultDragVisual = VisualContainer;
	DragOp->Pivot = EDragPivot::CenterCenter;

	SetVisibility(ESlateVisibility::Hidden);

	OutOperation = DragOp;
}

void UOWRPGInventoryItemWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetVisibility(ESlateVisibility::Visible);
}

void UOWRPGInventoryItemWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (BackgroundImage) BackgroundImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.8f));
}

void UOWRPGInventoryItemWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (BackgroundImage) BackgroundImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.3f));
}