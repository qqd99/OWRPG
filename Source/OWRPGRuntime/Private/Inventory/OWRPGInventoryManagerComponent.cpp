// Copyright Legion. All Rights Reserved.

#include "Inventory/OWRPGInventoryManagerComponent.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/InventoryFragment_Dimensions.h" 
#include "Inventory/OWRPGInventoryFragment_CoreStats.h" 
#include "Inventory/OWRPGInventoryFragment_Pickup.h" 
#include "Interaction/OWRPGWorldCollectable.h" 
#include "Inventory/OWRPGInventoryFunctionLibrary.h" 
#include "System/OWRPGGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// ==============================================================================
// FAST ARRAY
// ==============================================================================

void FOWRPGInventoryEntry::PostReplicatedChange(const FOWRPGInventoryList& InArraySerializer)
{
	if (UOWRPGInventoryManagerComponent* Manager = InArraySerializer.OwnerComponent)
	{
		Manager->OnEntryChanged(this);
	}
}

void FOWRPGInventoryList::PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters)
{
	if (OwnerComponent)
	{
		OwnerComponent->RequestUIUpdate();
	}
}

// ==============================================================================
// COMPONENT CORE
// ==============================================================================

UOWRPGInventoryManagerComponent::UOWRPGInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	PrimaryComponentTick.bCanEverTick = true;
	InventoryList.OwnerComponent = this;
}

void UOWRPGInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UOWRPGInventoryManagerComponent, InventoryList);
	DOREPLIFETIME(UOWRPGInventoryManagerComponent, Gold);
}

void UOWRPGInventoryManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bClientRefreshPending)
	{
		RebuildGrid();
		OnInventoryRefresh.Broadcast();
		bClientRefreshPending = false;
	}
}

void UOWRPGInventoryManagerComponent::RequestUIUpdate()
{
	bClientRefreshPending = true;
}

void UOWRPGInventoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	InventoryList.OwnerComponent = this;
	RebuildGrid();
}

void UOWRPGInventoryManagerComponent::OnRegister()
{
	Super::OnRegister();
	InventoryList.OwnerComponent = this;
}

// ==============================================================================
// SPATIAL GRID LOGIC
// ==============================================================================

void UOWRPGInventoryManagerComponent::RebuildGrid()
{
	int32 TotalSize = Rows * Columns;
	if (SpatialGrid.Num() != TotalSize)
	{
		SpatialGrid.SetNum(TotalSize);
	}

	FMemory::Memzero(SpatialGrid.GetData(), SpatialGrid.Num() * sizeof(TWeakObjectPtr<ULyraInventoryItemInstance>));

	for (const FOWRPGInventoryEntry& Entry : InventoryList.Entries)
	{
		if (!Entry.Item || Entry.X < 0 || Entry.Y < 0) continue;

		int32 W, H;
		GetItemDimensions(Entry.Item, W, H, Entry.bRotated);

		for (int32 x = Entry.X; x < Entry.X + W; x++)
		{
			for (int32 y = Entry.Y; y < Entry.Y + H; y++)
			{
				if (x >= 0 && x < Columns && y >= 0 && y < Rows)
				{
					int32 Index = y * Columns + x;
					SpatialGrid[Index] = Entry.Item;
				}
			}
		}
	}
}

ULyraInventoryItemInstance* UOWRPGInventoryManagerComponent::GetItemAt(int32 X, int32 Y) const
{
	if (X < 0 || X >= Columns || Y < 0 || Y >= Rows) return nullptr;
	int32 Index = Y * Columns + X;
	if (SpatialGrid.IsValidIndex(Index))
	{
		return SpatialGrid[Index].Get();
	}
	return nullptr;
}

TArray<ULyraInventoryItemInstance*> UOWRPGInventoryManagerComponent::GetItemsInRect(int32 StartX, int32 StartY, int32 Width, int32 Height) const
{
	TSet<ULyraInventoryItemInstance*> UniqueItems;
	TArray<ULyraInventoryItemInstance*> Result;

	// Boundary check
	if (StartX < 0 || StartY < 0 || (StartX + Width) > Columns || (StartY + Height) > Rows)
	{
		return Result;
	}

	// Optimized Spatial Lookup O(W*H)
	for (int32 x = StartX; x < StartX + Width; x++)
	{
		for (int32 y = StartY; y < StartY + Height; y++)
		{
			ULyraInventoryItemInstance* FoundItem = GetItemAt(x, y);
			if (FoundItem)
			{
				if (!UniqueItems.Contains(FoundItem))
				{
					UniqueItems.Add(FoundItem);
					Result.Add(FoundItem);
				}
			}
		}
	}
	return Result;
}

bool UOWRPGInventoryManagerComponent::IsRectFree(int32 StartX, int32 StartY, int32 Width, int32 Height, const TArray<ULyraInventoryItemInstance*>& IgnoredItems) const
{
	if (StartX < 0 || StartY < 0 || (StartX + Width) > Columns || (StartY + Height) > Rows)
	{
		return false;
	}

	for (int32 x = StartX; x < StartX + Width; x++)
	{
		for (int32 y = StartY; y < StartY + Height; y++)
		{
			ULyraInventoryItemInstance* FoundItem = GetItemAt(x, y);
			if (FoundItem != nullptr)
			{
				if (!IgnoredItems.Contains(FoundItem))
				{
					return false;
				}
			}
		}
	}
	return true;
}

void UOWRPGInventoryManagerComponent::OnEntryChanged(FOWRPGInventoryEntry* Entry)
{
	if (GetOwner()->HasAuthority())
	{
		RebuildGrid();
		OnInventoryRefresh.Broadcast();
	}
	else
	{
		RequestUIUpdate();
	}
}

// ==============================================================================
// HELPERS
// ==============================================================================

const FOWRPGInventoryEntry* UOWRPGInventoryManagerComponent::GetEntry(ULyraInventoryItemInstance* Item) const
{
	if (!Item) return nullptr;
	return InventoryList.Entries.FindByPredicate([&](const FOWRPGInventoryEntry& E) { return E.Item == Item; });
}

bool UOWRPGInventoryManagerComponent::Internal_RemoveItem(ULyraInventoryItemInstance* Item)
{
	int32 Idx = InventoryList.Entries.IndexOfByPredicate([&](const FOWRPGInventoryEntry& E) { return E.Item == Item; });
	if (Idx != INDEX_NONE)
	{
		InventoryList.Entries.RemoveAt(Idx);
		InventoryList.MarkArrayDirty();

		if (GetOwner()->HasAuthority())
		{
			RebuildGrid();
		}
		return true;
	}
	return false;
}

bool UOWRPGInventoryManagerComponent::Internal_AddItemInstance(ULyraInventoryItemInstance* Item, int32 X, int32 Y, bool bRotated)
{
	if (!Item) return false;

	RegisterReplication(Item);

	FOWRPGInventoryEntry& NewEntry = InventoryList.Entries.AddDefaulted_GetRef();
	NewEntry.Item = Item;
	NewEntry.X = X;
	NewEntry.Y = Y;
	NewEntry.bRotated = bRotated;

	InventoryList.MarkItemDirty(NewEntry);
	InventoryList.MarkArrayDirty();

	if (GetOwner()->HasAuthority())
	{
		RebuildGrid();
	}
	return true;
}

void UOWRPGInventoryManagerComponent::GetItemDimensions(const ULyraInventoryItemInstance* Item, int32& W, int32& H, bool bRotated) const
{
	W = 1; H = 1;
	if (!Item) return;

	const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(Item->GetItemDef());
	if (const UInventoryFragment_Dimensions* Frag = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UInventoryFragment_Dimensions>(Def))
	{
		W = Frag->Width;
		H = Frag->Height;
	}
	if (bRotated) { int32 T = W; W = H; H = T; }
}

bool UOWRPGInventoryManagerComponent::FindFreeSlot(ULyraInventoryItemInstance* Item, int32& OutX, int32& OutY)
{
	if (!Item) return false;
	int32 W, H;
	GetItemDimensions(Item, W, H, false);

	for (int32 y = 0; y <= Rows - H; y++)
	{
		for (int32 x = 0; x <= Columns - W; x++)
		{
			if (IsRectFree(x, y, W, H, {}))
			{
				OutX = x; OutY = y;
				return true;
			}
		}
	}
	return false;
}

float UOWRPGInventoryManagerComponent::GetTotalWeight() const
{
	float TotalWeight = 0.0f;
	for (const FOWRPGInventoryEntry& Entry : InventoryList.Entries)
	{
		if (Entry.Item)
		{
			const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(Entry.Item->GetItemDef());
			if (const UOWRPGInventoryFragment_CoreStats* Stats = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UOWRPGInventoryFragment_CoreStats>(Def))
			{
				int32 StackCount = UOWRPGInventoryFunctionLibrary::GetItemStatsStackCount(Entry.Item);
				if (StackCount <= 0) StackCount = 1;

				TotalWeight += (Stats->Weight * StackCount);
			}
		}
	}
	return TotalWeight;
}

// ==============================================================================
// ADD ITEM
// ==============================================================================
bool UOWRPGInventoryManagerComponent::AddItemDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount)
{
	if (!GetOwner()->HasAuthority() || !ItemDef || StackCount <= 0) return false;

	int32 MaxStack = 1;
	const ULyraInventoryItemDefinition* DefCDO = GetDefault<ULyraInventoryItemDefinition>(ItemDef);
	if (const UOWRPGInventoryFragment_CoreStats* StatsFrag = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UOWRPGInventoryFragment_CoreStats>(DefCDO))
	{
		MaxStack = StatsFrag->MaxStack;
	}

	// 1. PASS 1: Fill Existing Stacks
	for (FOWRPGInventoryEntry& Entry : InventoryList.Entries)
	{
		if (StackCount <= 0) break;
		if (!Entry.Item) continue;
		if (Entry.Item->GetItemDef() != ItemDef) continue;

		int32 CurrentStack = 0;
		// FIX: Use Library
		if (UOWRPGInventoryFunctionLibrary::HasItemStatsStack(Entry.Item))
		{
			CurrentStack = UOWRPGInventoryFunctionLibrary::GetItemStatsStackCount(Entry.Item);
		}
		else
		{
			// Initialize stack tag if missing
			UOWRPGInventoryFunctionLibrary::AddItemStatsStack(Entry.Item, 1);
			CurrentStack = 1;
		}

		if (CurrentStack < MaxStack)
		{
			int32 Space = MaxStack - CurrentStack;
			int32 Add = FMath::Min(StackCount, Space);

			// FIX: Use Library
			UOWRPGInventoryFunctionLibrary::AddItemStatsStack(Entry.Item, Add);
			StackCount -= Add;
			InventoryList.MarkItemDirty(Entry);
		}
	}

	if (StackCount <= 0)
	{
		InventoryList.MarkArrayDirty();
		return true;
	}

	// 2. PASS 2: Create New Stacks
	bool bAddedAny = false;
	while (StackCount > 0)
	{
		int32 AmountForThisSlot = FMath::Min(StackCount, MaxStack);

		ULyraInventoryItemInstance* NewItem = NewObject<ULyraInventoryItemInstance>(GetOwner());

		static FProperty* ItemDefProp = FindFProperty<FProperty>(ULyraInventoryItemInstance::StaticClass(), TEXT("ItemDef"));
		if (ItemDefProp)
		{
			UClass* DefClass = *ItemDef;
			if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(ItemDefProp))
			{
				ObjProp->SetObjectPropertyValue_InContainer(NewItem, DefClass);
			}
		}

		// FIX: Use Library
		UOWRPGInventoryFunctionLibrary::AddItemStatsStack(NewItem, AmountForThisSlot);

		int32 TargetX, TargetY;
		if (FindFreeSlot(NewItem, TargetX, TargetY))
		{
			Internal_AddItemInstance(NewItem, TargetX, TargetY, false);
			StackCount -= AmountForThisSlot;
			bAddedAny = true;
		}
		else
		{
			SpawnItemInWorld(NewItem, StackCount);
			return bAddedAny;
		}
	}

	return true;
}
// ==============================================================================
// DRAG AND DROP (ATOMIC SWAP)
// ==============================================================================

bool UOWRPGInventoryManagerComponent::ServerTransferItem_Validate(UOWRPGInventoryManagerComponent* SourceComponent, ULyraInventoryItemInstance* ItemInstance, int32 DestX, int32 DestY, bool bRotated) { return true; }
void UOWRPGInventoryManagerComponent::ServerTransferItem_Implementation(UOWRPGInventoryManagerComponent* SourceComponent, ULyraInventoryItemInstance* ItemInstance, int32 DestX, int32 DestY, bool bRotated)
{
	if (!SourceComponent || !ItemInstance) return;

	const FOWRPGInventoryEntry* SourceEntry = SourceComponent->GetEntry(ItemInstance);
	if (!SourceEntry) return;

	int32 SrcX = SourceEntry->X;
	int32 SrcY = SourceEntry->Y;
	bool SrcRot = SourceEntry->bRotated;

	int32 W, H;
	GetItemDimensions(ItemInstance, W, H, bRotated);

	// Boundary Check
	if (DestX < 0 || DestY < 0 || (DestX + W) > Columns || (DestY + H) > Rows)
	{
		return;
	}

	// Check for Collisions
	TSet<ULyraInventoryItemInstance*> Overlaps;
	for (int32 x = DestX; x < DestX + W; x++)
	{
		for (int32 y = DestY; y < DestY + H; y++)
		{
			ULyraInventoryItemInstance* Found = GetItemAt(x, y);
			if (Found && Found != ItemInstance)
			{
				Overlaps.Add(Found);
			}
		}
	}

	// --- SCENARIO 1: PLACE (No overlap) ---
	if (Overlaps.Num() == 0)
	{
		if (SourceComponent->Internal_RemoveItem(ItemInstance))
		{
			SourceComponent->UnregisterReplication(ItemInstance);
			Internal_AddItemInstance(ItemInstance, DestX, DestY, bRotated);
		}
		return;
	}

	// --- SCENARIO 2: STACK (1 Overlap, Same Type) ---
	if (Overlaps.Num() == 1)
	{
		ULyraInventoryItemInstance* TargetItem = Overlaps.Array()[0];
		if (TargetItem->GetItemDef() == ItemInstance->GetItemDef())
		{
			int32 SrcStack = UOWRPGInventoryFunctionLibrary::GetItemStatsStackCount(ItemInstance);
			int32 DstStack = UOWRPGInventoryFunctionLibrary::GetItemStatsStackCount(TargetItem);

			int32 MaxStack = 1;
			const ULyraInventoryItemDefinition* DefCDO = GetDefault<ULyraInventoryItemDefinition>(TargetItem->GetItemDef());
			if (const UOWRPGInventoryFragment_CoreStats* StatsFrag = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UOWRPGInventoryFragment_CoreStats>(DefCDO))
			{
				MaxStack = StatsFrag->MaxStack;
			}

			if (DstStack < MaxStack)
			{
				int32 Space = MaxStack - DstStack;
				int32 MoveAmount = FMath::Min(SrcStack, Space);

				if (MoveAmount > 0)
				{
					UOWRPGInventoryFunctionLibrary::AddItemStatsStack(TargetItem, MoveAmount);

					if (MoveAmount >= SrcStack)
					{
						SourceComponent->Internal_RemoveItem(ItemInstance);
						return;
					}
					else
					{
						UOWRPGInventoryFunctionLibrary::RemoveItemStatsStack(ItemInstance, MoveAmount);
						if (const FOWRPGInventoryEntry* SrcE = SourceComponent->GetEntry(ItemInstance))
						{
							FOWRPGInventoryEntry* MutableSrc = const_cast<FOWRPGInventoryEntry*>(SrcE);
							SourceComponent->InventoryList.MarkItemDirty(*MutableSrc);
						}

						if (const FOWRPGInventoryEntry* DstE = GetEntry(TargetItem))
						{
							FOWRPGInventoryEntry* MutableDst = const_cast<FOWRPGInventoryEntry*>(DstE);
							InventoryList.MarkItemDirty(*MutableDst);
						}
						return;
					}
				}
			}
		}
	}

	// --- SCENARIO 3: SWAP (Atomic Check) ---
	if (Overlaps.Num() == 1)
	{
		ULyraInventoryItemInstance* ItemB = Overlaps.Array()[0];

		int32 BW = 0;
		int32 BH = 0;
		int32 B_CurrentX = -1;
		int32 B_CurrentY = -1;
		bool B_CurrentRot = false;

		const FOWRPGInventoryEntry* EntryB = GetEntry(ItemB);
		if (EntryB)
		{
			B_CurrentX = EntryB->X;
			B_CurrentY = EntryB->Y;
			B_CurrentRot = EntryB->bRotated;
			GetItemDimensions(ItemB, BW, BH, B_CurrentRot);
		}

		// LOGIC FIX: Do A and B collide with each other in their NEW positions?
		// New A: DestX, DestY, W, H
		// New B: SrcX, SrcY, BW, BH
		bool bNewPositionsOverlap = false;
		if (DestX < SrcX + BW && DestX + W > SrcX && DestY < SrcY + BH && DestY + H > SrcY)
		{
			bNewPositionsOverlap = true;
		}

		// LOGIC FIX: Does B fit at Source? 
		// If Source == This (Same Inventory), we ignore A and B to check against empty space.
		// If Source != This (Different), we ignore A at source. B isn't there yet.
		bool bBFitsAtSource = false;

		if (!bNewPositionsOverlap)
		{
			if (SourceComponent == this)
			{
				bBFitsAtSource = SourceComponent->IsRectFree(SrcX, SrcY, BW, BH, { ItemInstance, ItemB });
			}
			else
			{
				bBFitsAtSource = SourceComponent->IsRectFree(SrcX, SrcY, BW, BH, { ItemInstance });
			}
		}

		if (bBFitsAtSource)
		{
			SourceComponent->Internal_RemoveItem(ItemInstance);
			Internal_RemoveItem(ItemB);

			if (SourceComponent != this)
			{
				SourceComponent->UnregisterReplication(ItemInstance);
				UnregisterReplication(ItemB);

				SourceComponent->RegisterReplication(ItemB);
				RegisterReplication(ItemInstance);
			}

			Internal_AddItemInstance(ItemInstance, DestX, DestY, bRotated);
			SourceComponent->Internal_AddItemInstance(ItemB, SrcX, SrcY, B_CurrentRot);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Swap Failed: Collision detected between Item A and B in new positions."));
		}
	}
}

// ==============================================================================
// PLAYER ACTIONS
// ==============================================================================

bool UOWRPGInventoryManagerComponent::ServerDropItem_Validate(ULyraInventoryItemInstance* Item) { return true; }
void UOWRPGInventoryManagerComponent::ServerDropItem_Implementation(ULyraInventoryItemInstance* Item)
{
	if (!Item) return;

	int32 Stack = UOWRPGInventoryFunctionLibrary::GetItemStatsStackCount(Item);
	if (Stack <= 0) Stack = 1;

	SpawnItemInWorld(Item, Stack);
	Internal_RemoveItem(Item);
}

bool UOWRPGInventoryManagerComponent::ServerSplitStack_Validate(ULyraInventoryItemInstance* Item, int32 AmountToSplit) { return true; }
void UOWRPGInventoryManagerComponent::ServerSplitStack_Implementation(ULyraInventoryItemInstance* Item, int32 AmountToSplit)
{
	if (!Item || AmountToSplit <= 0) return;

	int32 CurrentStack = UOWRPGInventoryFunctionLibrary::GetItemStatsStackCount(Item);

	if (CurrentStack <= AmountToSplit) return;

	UOWRPGInventoryFunctionLibrary::RemoveItemStatsStack(Item, AmountToSplit);
	if (const FOWRPGInventoryEntry* SrcEntry = GetEntry(Item))
	{
		FOWRPGInventoryEntry* MutableSrc = const_cast<FOWRPGInventoryEntry*>(SrcEntry);
		InventoryList.MarkItemDirty(*MutableSrc);
	}

	ULyraInventoryItemInstance* NewItem = NewObject<ULyraInventoryItemInstance>(GetOwner());

	static FProperty* ItemDefProp = FindFProperty<FProperty>(ULyraInventoryItemInstance::StaticClass(), TEXT("ItemDef"));
	if (ItemDefProp)
	{
		UClass* DefClass = *Item->GetItemDef();
		if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(ItemDefProp))
		{
			ObjProp->SetObjectPropertyValue_InContainer(NewItem, DefClass);
		}
	}

	UOWRPGInventoryFunctionLibrary::AddItemStatsStack(NewItem, AmountToSplit);

	int32 FreeX, FreeY;
	if (FindFreeSlot(NewItem, FreeX, FreeY))
	{
		Internal_AddItemInstance(NewItem, FreeX, FreeY, false);
	}
	else
	{
		SpawnItemInWorld(NewItem, AmountToSplit);
	}
}

bool UOWRPGInventoryManagerComponent::ServerEquipItem_Validate(ULyraInventoryItemInstance* Item) { return true; }
void UOWRPGInventoryManagerComponent::ServerEquipItem_Implementation(ULyraInventoryItemInstance* Item)
{
	if (!Item) return;
	if (Internal_RemoveItem(Item))
	{
		// TODO: Equipment Logic placeholder
	}
}

void UOWRPGInventoryManagerComponent::SpawnItemInWorld(ULyraInventoryItemInstance* Item, int32 StackCount)
{
	if (!GetOwner()) return;
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return;

	const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(Item->GetItemDef());
	if (const UOWRPGInventoryFragment_Pickup* PickupFrag = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UOWRPGInventoryFragment_Pickup>(Def))
	{
		if (PickupFrag->PickupActorClass)
		{
			FVector SpawnLoc = Pawn->GetActorLocation() + (Pawn->GetActorForwardVector() * 100.0f) + FVector(0, 0, 50.0f);
			FRotator RandomRot = Pawn->GetActorRotation();
			RandomRot.Yaw += FMath::RandRange(-20.0f, 20.0f);

			FTransform SpawnTransform(RandomRot, SpawnLoc);

			AOWRPGWorldCollectable* NewPickup = GetWorld()->SpawnActorDeferred<AOWRPGWorldCollectable>(
				PickupFrag->PickupActorClass,
				SpawnTransform,
				Pawn,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
			);

			if (NewPickup)
			{
				NewPickup->StaticItemDefinition = Item->GetItemDef();
				NewPickup->StackCount = StackCount;
				NewPickup->FinishSpawning(SpawnTransform);
			}
		}
	}

	UnregisterReplication(Item);
}

void UOWRPGInventoryManagerComponent::RegisterReplication(ULyraInventoryItemInstance* Item)
{
	if (Item && GetOwner()->HasAuthority())
	{
		AddReplicatedSubObject(Item);
	}
}

void UOWRPGInventoryManagerComponent::UnregisterReplication(ULyraInventoryItemInstance* Item)
{
	if (Item && GetOwner()->HasAuthority())
	{
		RemoveReplicatedSubObject(Item);
	}
}