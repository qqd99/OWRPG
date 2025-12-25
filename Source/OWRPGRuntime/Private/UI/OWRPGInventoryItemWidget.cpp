#include "UI/OWRPGInventoryItemWidget.h"
#include "Inventory/OWRPGInventoryManagerComponent.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "UI/OWRPGInventoryDragDrop.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h" 
#include "Inventory/OWRPGInventoryFragment_UI.h"

void UOWRPGInventoryItemWidget::UpdateVisual_Implementation()
{
	if (ItemInstance && InventoryManager)
	{
		// 1. Update Size
		int32 W, H;
		InventoryManager->GetItemDimensions(ItemInstance, W, H);
		float Width = W * TileSize;
		float Height = H * TileSize;
		SetDesiredSizeInViewport(FVector2D(Width, Height));

		// 2. Update Icon (The Fix)
		if (IconImage)
		{
			// Try to find the Icon Fragment
			const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(ItemInstance->GetItemDef());
			if (Def)
			{
				// Attempt to find the standard "SetIcon" fragment. 
				// If your project uses a different fragment for icons, change this class!
				if (const UOWRPGInventoryFragment_UI* IconFrag = FindObject<UOWRPGInventoryFragment_UI>(Def, TEXT("SetIcon"))) 
				{
					// If you have the fragment, set the brush
					// IconImage->SetBrushFromTexture(IconFrag->Icon);
				}

				// ALTERNATIVE: Use the Helper Library if you have one
				// UTexture2D* Icon = UOWRPGInventoryFunctionLibrary::GetItemIcon(Def);
				// if (Icon) IconImage->SetBrushFromTexture(Icon);
			}
		}
	}
}
// ... Rest of the file remains the same ...