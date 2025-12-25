// Copyright Legion. All Rights Reserved.

#include "Inventory/OWRPGInventoryManagerComponent.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/InventoryFragment_Dimensions.h" // Make sure you have this file!
#include "Inventory/OWRPGInventoryFunctionLibrary.h" // Helper to find fragments
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

// ==============================================================================
// FAST ARRAY IMPLEMENTATION
// ==============================================================================

void FOWRPGInventoryEntry::PostReplicatedChange(const FOWRPGInventoryList& InArraySerializer)
{
	// Client-side hook: When data changes, update the visual cache
	if (UOWRPGInventoryManagerComponent* Manager = InArraySerializer.OwnerComponent)
	{
		Manager->OnEntryChanged(this);
	}
}

// ==============================================================================
// MANAGER IMPLEMENTATION
// ==============================================================================

UOWRPGInventoryManagerComponent::UOWRPGInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	InventoryList.OwnerComponent = this;
}

void UOWRPGInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UOWRPGInventoryManagerComponent, InventoryList);
	DOREPLIFETIME(UOWRPGInventoryManagerComponent, CursorItem);
}

void UOWRPGInventoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize the memory for the Local Cache
	GridTiles.SetNum(Columns * Rows);
}

// ------------------------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------------------------

int32 UOWRPGInventoryManagerComponent::GetIndex(int32 X, int32 Y) const
{
	return (Y * Columns) + X;
}

void UOWRPGInventoryManagerComponent::GetItemDimensions(const ULyraInventoryItemInstance* Item, int32& W, int32& H) const
{
	W = 1; H = 1;
	if (!Item) return;

	const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(Item->GetItemDef());
	if (const UInventoryFragment_Dimensions* Frag = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UInventoryFragment_Dimensions>(Def))
	{
		W = Frag->Width;
		H = Frag->Height;
	}
}

// ------------------------------------------------------------------------------
// SERVER LOGIC (Pickup / Place / Swap)
// ------------------------------------------------------------------------------

void UOWRPGInventoryManagerComponent::Debug_AddItem(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 X, int32 Y)
{
	if (!ItemDef) return;

	// 1. Create the Object (Empty)
	ULyraInventoryItemInstance* NewItem = NewObject<ULyraInventoryItemInstance>(GetOwner());

	// 2. Set the "ItemDef" property using Reflection (Bypassing 'private')
	// We look for the property by name "ItemDef" and set the value manually.
	static FProperty* ItemDefProp = FindFProperty<FProperty>(ULyraInventoryItemInstance::StaticClass(), TEXT("ItemDef"));
	if (ItemDefProp)
	{
		// For TSubclassOf, it's stored as a UClass* in memory
		UClass* DefClass = *ItemDef;

		// This helper sets the property value on the NewItem instance
		// Note: FObjectPropertyBase handles TSubclassOf
		if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(ItemDefProp))
		{
			ObjProp->SetObjectPropertyValue_InContainer(NewItem, DefClass);
		}
	}

	// OR (Simpler Option if you decide to modify source later):
	// NewItem->SetItemDef(ItemDef); 

	// 3. Add to Grid logic
	FOWRPGInventoryEntry& NewEntry = InventoryList.Entries.AddDefaulted_GetRef();
	NewEntry.Item = NewItem;
	NewEntry.X = X;
	NewEntry.Y = Y;

	InventoryList.MarkItemDirty(NewEntry);
	InventoryList.MarkArrayDirty();
	RebuildCache();
}

// ------------------------------------------------------------------------------
// CACHE MANAGEMENT (The 0.03ms Rebuild)
// ------------------------------------------------------------------------------

void UOWRPGInventoryManagerComponent::OnEntryChanged(FOWRPGInventoryEntry* Entry)
{
	RebuildCache();
}

void UOWRPGInventoryManagerComponent::RebuildCache()
{
	// 1. Clear Cache
	for (FOWRPGInventoryTile& Tile : GridTiles)
	{
		Tile.Item = nullptr;
		Tile.bIsHead = false;
	}

	if (GridTiles.Num() != Columns * Rows)
	{
		GridTiles.SetNum(Columns * Rows);
	}

	// 2. Re-Paint Items
	for (const FOWRPGInventoryEntry& Entry : InventoryList.Entries)
	{
		if (!Entry.Item || Entry.X < 0 || Entry.Y < 0) continue;

		int32 W, H;
		GetItemDimensions(Entry.Item, W, H);

		for (int32 r = 0; r < H; r++)
		{
			for (int32 c = 0; c < W; c++)
			{
				int32 Index = GetIndex(Entry.X + c, Entry.Y + r);
				if (GridTiles.IsValidIndex(Index))
				{
					GridTiles[Index].Item = Entry.Item;
					GridTiles[Index].bIsHead = (r == 0 && c == 0);
				}
			}
		}
	}

	// 3. Notify UI
	OnInventoryVisualsChanged.Broadcast();
}

bool UOWRPGInventoryManagerComponent::AddItemDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount)
{
	if (!ItemDef) return false;

	// 1. Create the Instance (Using our Reflection workaround for now)
	ULyraInventoryItemInstance* NewItem = NewObject<ULyraInventoryItemInstance>(GetOwner());

	// Set ItemDef via Reflection
	static FProperty* ItemDefProp = FindFProperty<FProperty>(ULyraInventoryItemInstance::StaticClass(), TEXT("ItemDef"));
	if (ItemDefProp)
	{
		UClass* DefClass = *ItemDef;
		if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(ItemDefProp))
		{
			ObjProp->SetObjectPropertyValue_InContainer(NewItem, DefClass);
		}
	}

	// Set Stack Count (If your item supports it)
	// NewItem->SetStatTagStack(TAG_Lyra_Inventory_Stack, StackCount); // (Uncomment if you have this tag)

	// 2. Find a Spot
	int32 TargetX, TargetY;
	if (FindFreeSlot(NewItem, TargetX, TargetY))
	{
		// 3. Add to Data
		FOWRPGInventoryEntry& NewEntry = InventoryList.Entries.AddDefaulted_GetRef();
		NewEntry.Item = NewItem;
		NewEntry.X = TargetX;
		NewEntry.Y = TargetY;

		InventoryList.MarkItemDirty(NewEntry);
		InventoryList.MarkArrayDirty();

		// 4. Update Visuals
		RebuildCache();
		return true;
	}

	// Inventory Full!
	// (Optional: You could destroy NewItem here to avoid garbage leaks, though GC handles it eventually)
	return false;
}

bool UOWRPGInventoryManagerComponent::FindFreeSlot(ULyraInventoryItemInstance* Item, int32& OutX, int32& OutY) const
{
	if (!Item) return false;

	int32 W, H;
	GetItemDimensions(Item, W, H);

	// Brute Force Scan: Top-Left to Bottom-Right
	// (Since we have local cache 'GridTiles', this is super fast even for 10,000 slots)

	for (int32 y = 0; y <= Rows - H; y++)
	{
		for (int32 x = 0; x <= Columns - W; x++)
		{
			// Check if item fits at (x,y)
			bool bFits = true;

			// Inner Loop: Check the rect for the item
			for (int32 ItemY = 0; ItemY < H; ItemY++)
			{
				for (int32 ItemX = 0; ItemX < W; ItemX++)
				{
					int32 Index = GetIndex(x + ItemX, y + ItemY);

					// If slot is occupied by ANY item, we can't fit here
					if (GridTiles.IsValidIndex(Index) && GridTiles[Index].Item != nullptr)
					{
						bFits = false;
						break;
					}
				}
				if (!bFits) break;
			}

			if (bFits)
			{
				OutX = x;
				OutY = y;
				return true; // Found a spot!
			}
		}
	}

	return false; // No space found
}

void UOWRPGInventoryManagerComponent::OnRep_InventoryList()
{
	// Client received data -> Rebuild the visual cache
	RebuildCache();
}

// ==============================================================================
// SERVER RPC IMPLEMENTATION
// ==============================================================================

bool UOWRPGInventoryManagerComponent::ServerPickupItem_Validate(ULyraInventoryItemInstance* Item)
{
	return true; // Add anti-cheat checks here later
}

void UOWRPGInventoryManagerComponent::ServerPickupItem_Implementation(ULyraInventoryItemInstance* Item)
{
	// ... (Paste your EXACT existing ServerPickupItem logic here) ...
	// The logic inside is correct, we just moved it into the _Implementation function.

	if (!Item || CursorItem) return;

	int32 FoundIndex = -1;
	for (int32 i = 0; i < InventoryList.Entries.Num(); i++)
	{
		if (InventoryList.Entries[i].Item == Item)
		{
			FoundIndex = i;
			break;
		}
	}

	if (FoundIndex != -1)
	{
		CursorItem = InventoryList.Entries[FoundIndex].Item;
		InventoryList.Entries.RemoveAt(FoundIndex);
		InventoryList.MarkArrayDirty();
		RebuildCache();
	}
}

bool UOWRPGInventoryManagerComponent::ServerPlaceItem_Validate(int32 TargetX, int32 TargetY)
{
	return true;
}

void UOWRPGInventoryManagerComponent::ServerPlaceItem_Implementation(int32 TargetX, int32 TargetY)
{
	// ... (Paste your EXACT existing ServerPlaceItem logic here) ...
	// NOTE: Make sure you use the logic that handles Swapping!

	if (!CursorItem) return;

	int32 W, H;
	GetItemDimensions(CursorItem, W, H);

	if (TargetX < 0 || TargetY < 0 || (TargetX + W) > Columns || (TargetY + H) > Rows) return;

	// Collision Scan
	TArray<ULyraInventoryItemInstance*> Overlaps;
	for (int32 r = 0; r < H; r++)
	{
		for (int32 c = 0; c < W; c++)
		{
			int32 Index = GetIndex(TargetX + c, TargetY + r);
			if (GridTiles.IsValidIndex(Index) && GridTiles[Index].Item != nullptr)
			{
				Overlaps.AddUnique(GridTiles[Index].Item);
			}
		}
	}

	// Logic
	if (Overlaps.Num() == 0)
	{
		// Place
		FOWRPGInventoryEntry& NewEntry = InventoryList.Entries.AddDefaulted_GetRef();
		NewEntry.Item = CursorItem;
		NewEntry.X = TargetX;
		NewEntry.Y = TargetY;
		InventoryList.MarkItemDirty(NewEntry);
		InventoryList.MarkArrayDirty();
		CursorItem = nullptr;
	}
	else if (Overlaps.Num() == 1)
	{
		// Swap
		ULyraInventoryItemInstance* OtherItem = Overlaps[0];
		int32 OtherIndex = -1;
		for (int32 i = 0; i < InventoryList.Entries.Num(); i++)
		{
			if (InventoryList.Entries[i].Item == OtherItem) { OtherIndex = i; break; }
		}

		if (OtherIndex != -1)
		{
			InventoryList.Entries.RemoveAt(OtherIndex);

			FOWRPGInventoryEntry& NewEntry = InventoryList.Entries.AddDefaulted_GetRef();
			NewEntry.Item = CursorItem;
			NewEntry.X = TargetX;
			NewEntry.Y = TargetY;
			CursorItem = OtherItem; // Swap hands
			InventoryList.MarkArrayDirty();
		}
	}

	RebuildCache();
}