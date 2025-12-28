// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "OWRPGInventoryManagerComponent.generated.h"

class UOWRPGInventoryManagerComponent;

// -----------------------------------------------------------------------------------
// FAST ARRAY (Network Data)
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

	UPROPERTY()
	bool bRotated = false;

	void PostReplicatedChange(const struct FOWRPGInventoryList& InArraySerializer);
};

USTRUCT(BlueprintType)
struct FOWRPGInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FOWRPGInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UOWRPGInventoryManagerComponent> OwnerComponent;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FOWRPGInventoryEntry, FOWRPGInventoryList>(Entries, DeltaParms, *this);
	}

	void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters);
};

template<>
struct TStructOpsTypeTraits<FOWRPGInventoryList> : public TStructOpsTypeTraitsBase2<FOWRPGInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

// -----------------------------------------------------------------------------------
// MANAGER COMPONENT
// -----------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryRefresh);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OWRPGRUNTIME_API UOWRPGInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOWRPGInventoryManagerComponent(const FObjectInitializer& ObjectInitializer);

	// --- CONFIG ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Columns = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Rows = 6;

	// --- STATE ---

	UPROPERTY(Replicated)
	FOWRPGInventoryList InventoryList;

	/** The Spatial Cache. O(1) Lookups. NOT Replicated. */
	TArray<TWeakObjectPtr<ULyraInventoryItemInstance>> SpatialGrid;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	int32 Gold = 0;

	UPROPERTY(BlueprintAssignable)
	FOnInventoryRefresh OnInventoryRefresh;

	// --- LIFECYCLE ---
	virtual void BeginPlay() override;
	virtual void OnRegister() override;

	// --- LOGIC ---

	void RebuildGrid();

	/** Gets the item at a specific grid coordinate. O(1) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	ULyraInventoryItemInstance* GetItemAt(int32 X, int32 Y) const;

	/** Returns all unique items overlapping the specified rectangle. O(W*H) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<ULyraInventoryItemInstance*> GetItemsInRect(int32 StartX, int32 StartY, int32 Width, int32 Height) const;

	/** Checks if a rectangle is free. */
	bool IsRectFree(int32 StartX, int32 StartY, int32 Width, int32 Height, const TArray<ULyraInventoryItemInstance*>& IgnoredItems) const;

	// --- REPLICATION ---
	void RegisterReplication(ULyraInventoryItemInstance* Item);
	void UnregisterReplication(ULyraInventoryItemInstance* Item);

	// --- ACTIONS ---

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool AddItemDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	/**
	 * Main function for Drag & Drop.
	 * Handles: Move within same inventory, Move between containers, Swapping, Stacking.
	 */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerTransferItem(UOWRPGInventoryManagerComponent* SourceComponent, ULyraInventoryItemInstance* ItemInstance, int32 DestX, int32 DestY, bool bRotated);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerDropItem(ULyraInventoryItemInstance* Item);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerSplitStack(ULyraInventoryItemInstance* Item, int32 AmountToSplit);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerEquipItem(ULyraInventoryItemInstance* Item);

	// --- HELPERS ---
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	float GetTotalWeight() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetTotalGold() const { return Gold; }

	bool FindFreeSlot(ULyraInventoryItemInstance* Item, int32& OutX, int32& OutY);

	// Internal Low-Level Manipulation (Updates Grid & Array)
	bool Internal_AddItemInstance(ULyraInventoryItemInstance* Item, int32 X, int32 Y, bool bRotated);
	bool Internal_RemoveItem(ULyraInventoryItemInstance* Item);

	const FOWRPGInventoryEntry* GetEntry(ULyraInventoryItemInstance* Item) const;

	void GetItemDimensions(const ULyraInventoryItemInstance* Item, int32& W, int32& H, bool bRotated) const;
	void OnEntryChanged(FOWRPGInventoryEntry* Entry);

	void RequestUIUpdate();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SpawnItemInWorld(ULyraInventoryItemInstance* Item, int32 StackCount);

	bool bClientRefreshPending = false;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};