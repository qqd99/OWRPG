// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "OWRPGInventoryManagerComponent.generated.h"

class UOWRPGInventoryManagerComponent;

// -----------------------------------------------------------------------------------
// 1. FAST ARRAY (The "Truth" - Networked)
// -----------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FOWRPGInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ULyraInventoryItemInstance> Item = nullptr;

	UPROPERTY()
	int32 X = -1;

	UPROPERTY()
	int32 Y = -1;

	// Called on Client when this specific entry updates
	void PostReplicatedChange(const struct FOWRPGInventoryList& InArraySerializer);
};

USTRUCT(BlueprintType)
struct FOWRPGInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FOWRPGInventoryEntry> Entries;

	// Automatically set to the component that owns this list
	UPROPERTY(NotReplicated)
	TObjectPtr<UOWRPGInventoryManagerComponent> OwnerComponent;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FOWRPGInventoryEntry, FOWRPGInventoryList>(Entries, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FOWRPGInventoryList> : public TStructOpsTypeTraitsBase2<FOWRPGInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

// -----------------------------------------------------------------------------------
// 2. LOCAL CACHE (The "Speed" - Non-Networked)
// -----------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FOWRPGInventoryTile
{
	GENERATED_BODY()

	// Pointer to the item (read from Fast Array)
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ULyraInventoryItemInstance> Item = nullptr;

	// Is this the top-left tile of the item? (For UI rendering)
	UPROPERTY(BlueprintReadOnly)
	bool bIsHead = false;
};

// -----------------------------------------------------------------------------------
// 3. THE MANAGER COMPONENT
// -----------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryVisualsChanged);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OWRPGRUNTIME_API UOWRPGInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOWRPGInventoryManagerComponent(const FObjectInitializer& ObjectInitializer);

	// --- CONFIGURATION ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Columns = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Rows = 10;

	// --- STATE (Networked) ---
	UPROPERTY(ReplicatedUsing = OnRep_InventoryList)
	FOWRPGInventoryList InventoryList;

	// The item currently held on the cursor (Floating)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<ULyraInventoryItemInstance> CursorItem = nullptr;

	// --- STATE (Local Cache) ---
	// The O(1) Lookup Table. Rebuilt automatically.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FOWRPGInventoryTile> GridTiles;

	// --- EVENTS ---
	UPROPERTY(BlueprintAssignable)
	FOnInventoryVisualsChanged OnInventoryVisualsChanged;

	// --- ACTIONS API (Server Only) ---

	// Pick up an item from the grid (Move Grid -> Cursor)
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerPickupItem(ULyraInventoryItemInstance* Item);

	// Drop the cursor item into the grid (Move Cursor -> Grid)
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerPlaceItem(int32 TargetX, int32 TargetY);

	// Tries to add an item definition to the first available slot (Auto-Loot)
	// Returns true if successful
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool AddItemDefinition(TSubclassOf<class ULyraInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	// Helper to find the first empty space for an item
	bool FindFreeSlot(ULyraInventoryItemInstance* Item, int32& OutX, int32& OutY) const;

	// Debug: Add item directly to grid
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Debug")
	void Debug_AddItem(TSubclassOf<class ULyraInventoryItemDefinition> ItemDef, int32 X, int32 Y);

	// --- INTERNAL / HELPERS ---

	// Called by Fast Array
	void OnEntryChanged(FOWRPGInventoryEntry* Entry);

	// Rebuilds GridTiles from InventoryList
	void RebuildCache();

	// Helper: Convert X/Y to Index
	int32 GetIndex(int32 X, int32 Y) const;

	// Helper: Get Size of an item
	void GetItemDimensions(const ULyraInventoryItemInstance* Item, int32& W, int32& H) const;

	// Catch-all for network updates
	UFUNCTION()
	void OnRep_InventoryList();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
};