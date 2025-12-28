// Copyright Legion. All Rights Reserved.

#include "UI/OWRPGInventoryDragDrop.h"
#include "Inventory/LyraInventoryItemInstance.h"

ULyraInventoryItemInstance* UOWRPGInventoryDragDrop::GetDraggedItem() const
{
	return DraggedItem.Get();
}