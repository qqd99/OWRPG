// Copyright Legion. All Rights Reserved.

#include "UI/OWRPGInventoryItemWidget.h"
#include "Inventory/OWRPGInventoryManagerComponent.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/OWRPGInventoryFragment_UI.h" 
#include "Inventory/OWRPGInventoryFunctionLibrary.h"
#include "System/OWRPGGameplayTags.h"
#include "UI/OWRPGInventoryDragDrop.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h" 
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

void UOWRPGInventoryItemWidget::UpdateVisual_Implementation()
{
	if (!ItemInstance || !InventoryManager) return;

	// 1. Calculate Size based on Dimensions and Rotation
	int32 W, H;
	InventoryManager->GetItemDimensions(ItemInstance, W, H, bIsRotated);

	float PixelW = W * TileSize;
	float PixelH = H * TileSize;

	// 2. Apply Size
	SetDesiredSizeInViewport(FVector2D(PixelW, PixelH));
	if (USizeBox* RootSizeBox = Cast<USizeBox>(GetRootWidget()))
	{
		RootSizeBox->SetWidthOverride(PixelW);
		RootSizeBox->SetHeightOverride(PixelH);
	}

	// 3. Apply Icon
	if (IconImage)
	{
		// Render Transform for Rotation (90 degrees if rotated)
		if (bIsRotated)
		{
			IconImage->SetRenderTransformAngle(90.0f);
		}
		else
		{
			IconImage->SetRenderTransformAngle(0.0f);
		}

		// Find Icon
		const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(ItemInstance->GetItemDef());
		if (Def)
		{
			if (const UOWRPGInventoryFragment_UI* UIFrag = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UOWRPGInventoryFragment_UI>(Def))
			{
				if (UIFrag->Icon)
				{
					IconImage->SetBrushFromTexture(UIFrag->Icon);
				}
			}
		}
	}

	// 4. Apply Stack Count
	if (StackText)
	{
		int32 Count = 0;
		// Reflection call to GetStatTagStackCount
		static UFunction* GetStackFunc = ItemInstance->FindFunction(TEXT("GetStatTagStackCount"));
		if (GetStackFunc)
		{
			struct FParams { FGameplayTag Tag; int32 ReturnValue; };
			FParams Params;
			Params.Tag = OWRPGGameplayTags::OWRPG_Inventory_Stack;
			Params.ReturnValue = 0;
			ItemInstance->ProcessEvent(GetStackFunc, &Params);
			Count = Params.ReturnValue;
		}

		if (Count > 1)
		{
			StackText->SetText(FText::AsNumber(Count));
			StackText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			StackText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UOWRPGInventoryItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!ItemInstance || !InventoryManager) return;

	// 1. Tell Server: Pickup Logic
	InventoryManager->ServerPickupItem(ItemInstance);

	// 2. Create Drag Payload
	UOWRPGInventoryDragDrop* DragOp = NewObject<UOWRPGInventoryDragDrop>();
	DragOp->DraggedItem = ItemInstance;
	DragOp->SourceWidget = this;
	DragOp->bIsRotated = bIsRotated; // Pass current rotation state

	// Cache Dimensions for Grid
	InventoryManager->GetItemDimensions(ItemInstance, DragOp->DraggedItemW, DragOp->DraggedItemH, bIsRotated);

	// Calculate Mouse Offset (So item doesn't snap to 0,0 relative to cursor)
	DragOp->DragOffset = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

	// 3. Create Visual (Ghost)
	UOWRPGInventoryItemWidget* DragVisual = CreateWidget<UOWRPGInventoryItemWidget>(GetOwningPlayer(), GetClass());
	if (DragVisual)
	{
		DragVisual->ItemInstance = ItemInstance;
		DragVisual->InventoryManager = InventoryManager;
		DragVisual->TileSize = this->TileSize;
		DragVisual->bIsRotated = this->bIsRotated; // Keep visual rotation
		DragVisual->UpdateVisual();
	}

	DragOp->DefaultDragVisual = DragVisual;
	DragOp->Pivot = EDragPivot::MouseDown;

	// 4. Hide original
	SetVisibility(ESlateVisibility::HitTestInvisible); // Or Hidden/Collapsed depending on preference

	OutOperation = DragOp;
}

void UOWRPGInventoryItemWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	// Create Tooltip
	if (TooltipWidgetClass && !GetToolTip())
	{
		UUserWidget* TooltipWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), TooltipWidgetClass);
		// Note: You can cast TooltipWidget to a custom class and pass ItemInstance here
		SetToolTip(TooltipWidget);
	}
}

void UOWRPGInventoryItemWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UOWRPGInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Right Click -> Context Menu
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		CreateContextMenu();
		return FReply::Handled();
	}

	// Left Click -> Start Drag Detection
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UOWRPGInventoryItemWidget::CreateContextMenu()
{
	if (!ContextMenuClass || !ItemInstance) return;

	// 1. Create Menu Widget
	UUserWidget* Menu = CreateWidget<UUserWidget>(GetOwningPlayer(), ContextMenuClass);
	if (Menu)
	{
		// 2. Add to Viewport
		Menu->AddToViewport(100); // High Z-Order

		// 3. Position at Mouse
		FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
		Menu->SetPositionInViewport(MousePos, false);

		// 4. Pass Data to BP
		OnContextOpened(Menu, ItemInstance);
	}
}