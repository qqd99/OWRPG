// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "OWRPGInventoryManagerComponent.generated.h"

class UOWRPGInventoryManagerComponent;

// -----------------------------------------------------------------------------------
// FAST ARRAY
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

	// CRITICAL FIX: Ensures UI updates on Add/Remove (Client Side)
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

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<ULyraInventoryItemInstance> CursorItem;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	int32 Gold = 0;

	UPROPERTY(BlueprintAssignable)
	FOnInventoryRefresh OnInventoryRefresh;

	// --- LIFECYCLE ---
	virtual void BeginPlay() override;
	virtual void OnRegister() override;

	// --- REPLICATION ---
	// Helper to ensure an item is replicating
	void RegisterReplication(ULyraInventoryItemInstance* Item);

	// Helper to stop replicating an item (e.g. when dropped)
	void UnregisterReplication(ULyraInventoryItemInstance* Item);

	// --- CROSS-INVENTORY TRANSFER ---
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerTransferFrom(UOWRPGInventoryManagerComponent* SourceManager, ULyraInventoryItemInstance* SourceItem, int32 DestX, int32 DestY, bool bRotated);

	// --- SINGLE INVENTORY ACTIONS ---

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool AddItemDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerAttemptMove(ULyraInventoryItemInstance* Item, int32 DestX, int32 DestY, bool bDestRotated);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerPickupItem(ULyraInventoryItemInstance* Item);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerDropFromGrid(ULyraInventoryItemInstance* Item);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void ServerDropFromCursor();

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

	// INTERNAL: Adds an item instance directly
	bool Internal_AddItemInstance(ULyraInventoryItemInstance* Item, int32 X, int32 Y, bool bRotated);

	// INTERNAL: Removes item from list but does NOT destroy it
	bool Internal_RemoveItem(ULyraInventoryItemInstance* Item);

	// INTERNAL: Gets the entry data (X, Y)
	const FOWRPGInventoryEntry* GetEntry(ULyraInventoryItemInstance* Item) const;

	TArray<ULyraInventoryItemInstance*> GetItemsInRect(int32 X, int32 Y, int32 W, int32 H, ULyraInventoryItemInstance* ExcludeItem) const;
	void GetItemDimensions(const ULyraInventoryItemInstance* Item, int32& W, int32& H, bool bRotated) const;
	void OnEntryChanged(FOWRPGInventoryEntry* Entry);

	// Debug
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Debug")
	void Debug_AddItem(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount = 1);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SpawnItemInWorld(ULyraInventoryItemInstance* Item, int32 StackCount);
};