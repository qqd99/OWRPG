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

// --- Fast Array ---
void FOWRPGInventoryEntry::PostReplicatedChange(const FOWRPGInventoryList& InArraySerializer)
{
	if (UOWRPGInventoryManagerComponent* Manager = InArraySerializer.OwnerComponent)
	{
		Manager->OnEntryChanged(this);
	}
}

// --- Component ---
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
	DOREPLIFETIME(UOWRPGInventoryManagerComponent, Gold);
}

void UOWRPGInventoryManagerComponent::OnEntryChanged(FOWRPGInventoryEntry* Entry)
{
	OnInventoryRefresh.Broadcast();
}

// ==============================================================================
// INTERNAL HELPERS
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
		return true;
	}
	return false;
}

bool UOWRPGInventoryManagerComponent::Internal_AddItemInstance(ULyraInventoryItemInstance* Item, int32 X, int32 Y, bool bRotated)
{
	if (!Item) return false;

	FOWRPGInventoryEntry& NewEntry = InventoryList.Entries.AddDefaulted_GetRef();
	NewEntry.Item = Item;
	NewEntry.X = X;
	NewEntry.Y = Y;
	NewEntry.bRotated = bRotated;

	InventoryList.MarkItemDirty(NewEntry);
	InventoryList.MarkArrayDirty();
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

TArray<ULyraInventoryItemInstance*> UOWRPGInventoryManagerComponent::GetItemsInRect(int32 TargetX, int32 TargetY, int32 RectW, int32 RectH, ULyraInventoryItemInstance* ExcludeItem) const
{
	TArray<ULyraInventoryItemInstance*> Overlaps;
	for (const FOWRPGInventoryEntry& Entry : InventoryList.Entries)
	{
		if (!Entry.Item || Entry.Item == ExcludeItem) continue;

		int32 EntryW, EntryH;
		GetItemDimensions(Entry.Item, EntryW, EntryH, Entry.bRotated);

		bool bOverlapX = (TargetX < Entry.X + EntryW) && (TargetX + RectW > Entry.X);
		bool bOverlapY = (TargetY < Entry.Y + EntryH) && (TargetY + RectH > Entry.Y);

		if (bOverlapX && bOverlapY)
		{
			Overlaps.Add(Entry.Item);
		}
	}
	return Overlaps;
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
			if (GetItemsInRect(x, y, W, H, nullptr).Num() == 0)
			{
				OutX = x; OutY = y; return true;
			}
		}
	}
	return false;
}

// ==============================================================================
// ADD ITEM (SMART STACKING)
// ==============================================================================

bool UOWRPGInventoryManagerComponent::AddItemDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount)
{
	if (!GetOwner()->HasAuthority() || !ItemDef || StackCount <= 0) return false;

	// 1. Determine Max Stack
	int32 MaxStack = 1;
	const ULyraInventoryItemDefinition* DefCDO = GetDefault<ULyraInventoryItemDefinition>(ItemDef);
	if (const UOWRPGInventoryFragment_CoreStats* StatsFrag = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UOWRPGInventoryFragment_CoreStats>(DefCDO))
	{
		MaxStack = StatsFrag->MaxStack;
	}

	// 2. PASS 1: Fill Existing Stacks
	// FIX: Removed 'const' so we can call MarkItemDirty
	for (FOWRPGInventoryEntry& Entry : InventoryList.Entries)
	{
		if (StackCount <= 0) break;
		if (!Entry.Item) continue;

		if (Entry.Item->GetItemDef() != ItemDef) continue;

		int32 CurrentStack = 0;
		static UFunction* GetStackFunc = Entry.Item->FindFunction(TEXT("GetStatTagStackCount"));
		if (GetStackFunc)
		{
			struct FParams { FGameplayTag Tag; int32 ReturnValue; };
			FParams Params = { OWRPGGameplayTags::OWRPG_Inventory_Stack, 0 };
			Entry.Item->ProcessEvent(GetStackFunc, &Params);
			CurrentStack = Params.ReturnValue;
		}

		if (CurrentStack < MaxStack)
		{
			int32 Space = MaxStack - CurrentStack;
			int32 Add = FMath::Min(StackCount, Space);

			static UFunction* AddStackFunc = Entry.Item->FindFunction(TEXT("AddStatTagStack"));
			if (AddStackFunc)
			{
				struct FParams { FGameplayTag Tag; int32 Count; };
				FParams Params = { OWRPGGameplayTags::OWRPG_Inventory_Stack, Add };
				Entry.Item->ProcessEvent(AddStackFunc, &Params);
			}

			StackCount -= Add;
			InventoryList.MarkItemDirty(Entry); // Now works because Entry is not const
		}
	}

	if (StackCount <= 0)
	{
		InventoryList.MarkArrayDirty();
		return true;
	}

	// 3. PASS 2: Create New Stacks
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

		static UFunction* AddStackFunc = NewItem->FindFunction(TEXT("AddStatTagStack"));
		if (AddStackFunc)
		{
			struct FParams { FGameplayTag Tag; int32 Count; };
			FParams Params = { OWRPGGameplayTags::OWRPG_Inventory_Stack, AmountForThisSlot };
			NewItem->ProcessEvent(AddStackFunc, &Params);
		}

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
// PLAYER ACTIONS
// ==============================================================================

bool UOWRPGInventoryManagerComponent::ServerTransferFrom_Validate(UOWRPGInventoryManagerComponent* SourceManager, ULyraInventoryItemInstance* SourceItem, int32 DestX, int32 DestY, bool bRotated) { return true; }
void UOWRPGInventoryManagerComponent::ServerTransferFrom_Implementation(UOWRPGInventoryManagerComponent* SourceManager, ULyraInventoryItemInstance* SourceItem, int32 DestX, int32 DestY, bool bRotated)
{
	if (!SourceManager || !SourceItem) return;
	if (SourceManager == this)
	{
		ServerAttemptMove(SourceItem, DestX, DestY, bRotated);
		return;
	}

	const FOWRPGInventoryEntry* SourceEntry = SourceManager->GetEntry(SourceItem);
	if (!SourceEntry) return;

	int32 OriginalSrcX = SourceEntry->X;
	int32 OriginalSrcY = SourceEntry->Y;
	bool OriginalSrcRot = SourceEntry->bRotated;

	int32 W, H;
	GetItemDimensions(SourceItem, W, H, bRotated);
	if (DestX < 0 || DestY < 0 || (DestX + W) > Columns || (DestY + H) > Rows) return;

	TArray<ULyraInventoryItemInstance*> Overlaps = GetItemsInRect(DestX, DestY, W, H, nullptr);

	if (Overlaps.Num() == 0)
	{
		// EMPTY -> PLACE
		if (SourceManager->Internal_RemoveItem(SourceItem))
		{
			Internal_AddItemInstance(SourceItem, DestX, DestY, bRotated);
		}
	}
	else if (Overlaps.Num() == 1)
	{
		ULyraInventoryItemInstance* DestItem = Overlaps[0];

		// --- STACKING CHECK ---
		if (DestItem->GetItemDef() == SourceItem->GetItemDef())
		{
			// 1. Get Stats
			int32 SrcStack = 0;
			int32 DstStack = 0;
			int32 MaxStack = 1;

			// Get MaxStack
			const ULyraInventoryItemDefinition* DefCDO = GetDefault<ULyraInventoryItemDefinition>(DestItem->GetItemDef());
			if (const UOWRPGInventoryFragment_CoreStats* StatsFrag = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UOWRPGInventoryFragment_CoreStats>(DefCDO))
			{
				MaxStack = StatsFrag->MaxStack;
			}

			// Get Counts
			static UFunction* GetStackFunc = SourceItem->FindFunction(TEXT("GetStatTagStackCount"));
			if (GetStackFunc)
			{
				struct FParams { FGameplayTag Tag; int32 ReturnValue; };
				FParams ParamsSrc = { OWRPGGameplayTags::OWRPG_Inventory_Stack, 0 };
				SourceItem->ProcessEvent(GetStackFunc, &ParamsSrc);
				SrcStack = ParamsSrc.ReturnValue;

				FParams ParamsDst = { OWRPGGameplayTags::OWRPG_Inventory_Stack, 0 };
				DestItem->ProcessEvent(GetStackFunc, &ParamsDst);
				DstStack = ParamsDst.ReturnValue;
			}

			// 2. Transfer
			if (DstStack < MaxStack)
			{
				int32 Space = MaxStack - DstStack;
				int32 MoveAmount = FMath::Min(SrcStack, Space);

				if (MoveAmount > 0)
				{
					// Add to Dest
					static UFunction* AddStackFunc = DestItem->FindFunction(TEXT("AddStatTagStack"));
					if (AddStackFunc)
					{
						struct FParams { FGameplayTag Tag; int32 Count; };
						FParams Params = { OWRPGGameplayTags::OWRPG_Inventory_Stack, MoveAmount };
						DestItem->ProcessEvent(AddStackFunc, &Params);
					}

					// Remove from Source
					if (MoveAmount >= SrcStack)
					{
						SourceManager->Internal_RemoveItem(SourceItem);
					}
					else
					{
						static UFunction* RemoveStackFunc = SourceItem->FindFunction(TEXT("RemoveStatTagStack"));
						if (RemoveStackFunc)
						{
							struct FParams { FGameplayTag Tag; int32 Count; };
							FParams Params = { OWRPGGameplayTags::OWRPG_Inventory_Stack, MoveAmount };
							SourceItem->ProcessEvent(RemoveStackFunc, &Params);
						}
						// Mark Source Entry Dirty
						if (const FOWRPGInventoryEntry* SrcE = SourceManager->GetEntry(SourceItem))
						{
							FOWRPGInventoryEntry* MutableSrcE = const_cast<FOWRPGInventoryEntry*>(SrcE);
							SourceManager->InventoryList.MarkItemDirty(*MutableSrcE);
						}
					}

					// Mark Dest Entry Dirty
					if (const FOWRPGInventoryEntry* DstE = GetEntry(DestItem))
					{
						FOWRPGInventoryEntry* MutableDstE = const_cast<FOWRPGInventoryEntry*>(DstE);
						InventoryList.MarkItemDirty(*MutableDstE);
					}
				}
			}
		}
		else
		{
			// --- SWAP ---
			int32 B_W, B_H;
			SourceManager->GetItemDimensions(DestItem, B_W, B_H, OriginalSrcRot);

			if (OriginalSrcX < 0 || OriginalSrcY < 0 || (OriginalSrcX + B_W) > SourceManager->Columns || (OriginalSrcY + B_H) > SourceManager->Rows) return;

			TArray<ULyraInventoryItemInstance*> SourceOverlaps = SourceManager->GetItemsInRect(OriginalSrcX, OriginalSrcY, B_W, B_H, SourceItem);

			if (SourceOverlaps.Num() == 0)
			{
				SourceManager->Internal_RemoveItem(SourceItem);
				this->Internal_RemoveItem(DestItem);

				this->Internal_AddItemInstance(SourceItem, DestX, DestY, bRotated);
				SourceManager->Internal_AddItemInstance(DestItem, OriginalSrcX, OriginalSrcY, OriginalSrcRot);
			}
		}
	}
}

// --- STANDARD ACTIONS ---

void UOWRPGInventoryManagerComponent::Debug_AddItem(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount)
{
	AddItemDefinition(ItemDef, StackCount);
}

bool UOWRPGInventoryManagerComponent::ServerPickupItem_Validate(ULyraInventoryItemInstance* Item) { return true; }
void UOWRPGInventoryManagerComponent::ServerPickupItem_Implementation(ULyraInventoryItemInstance* Item)
{
	if (!Item || CursorItem) return;
	if (Internal_RemoveItem(Item))
	{
		CursorItem = Item;
	}
}

bool UOWRPGInventoryManagerComponent::ServerAttemptMove_Validate(ULyraInventoryItemInstance* Item, int32 DestX, int32 DestY, bool bDestRotated) { return true; }
void UOWRPGInventoryManagerComponent::ServerAttemptMove_Implementation(ULyraInventoryItemInstance* Item, int32 DestX, int32 DestY, bool bDestRotated)
{
	if (CursorItem && CursorItem == Item)
	{
		int32 W, H;
		GetItemDimensions(CursorItem, W, H, bDestRotated);

		if (DestX < 0 || DestY < 0 || (DestX + W) > Columns || (DestY + H) > Rows) return;

		TArray<ULyraInventoryItemInstance*> Overlaps = GetItemsInRect(DestX, DestY, W, H, nullptr);

		if (Overlaps.Num() == 0)
		{
			Internal_AddItemInstance(CursorItem, DestX, DestY, bDestRotated);
			CursorItem = nullptr;
		}
		else if (Overlaps.Num() == 1)
		{
			ULyraInventoryItemInstance* OtherItem = Overlaps[0];
			if (Internal_RemoveItem(OtherItem))
			{
				Internal_AddItemInstance(CursorItem, DestX, DestY, bDestRotated);
				CursorItem = OtherItem;
			}
		}
	}
}

bool UOWRPGInventoryManagerComponent::ServerDropFromCursor_Validate() { return true; }
void UOWRPGInventoryManagerComponent::ServerDropFromCursor_Implementation()
{
	if (CursorItem)
	{
		int32 Stack = 1;
		static UFunction* GetStackFunc = CursorItem->FindFunction(TEXT("GetStatTagStackCount"));
		if (GetStackFunc)
		{
			struct FParams { FGameplayTag Tag; int32 ReturnValue; };
			FParams Params = { OWRPGGameplayTags::OWRPG_Inventory_Stack, 0 };
			CursorItem->ProcessEvent(GetStackFunc, &Params);
			Stack = Params.ReturnValue;
		}
		SpawnItemInWorld(CursorItem, Stack);
		CursorItem = nullptr;
	}
}

bool UOWRPGInventoryManagerComponent::ServerDropFromGrid_Validate(ULyraInventoryItemInstance* Item) { return true; }
void UOWRPGInventoryManagerComponent::ServerDropFromGrid_Implementation(ULyraInventoryItemInstance* Item)
{
	int32 Idx = InventoryList.Entries.IndexOfByPredicate([&](const FOWRPGInventoryEntry& E) { return E.Item == Item; });
	if (Idx != INDEX_NONE)
	{
		int32 Stack = 1;
		static UFunction* GetStackFunc = Item->FindFunction(TEXT("GetStatTagStackCount"));
		if (GetStackFunc)
		{
			struct FParams { FGameplayTag Tag; int32 ReturnValue; };
			FParams Params = { OWRPGGameplayTags::OWRPG_Inventory_Stack, 0 };
			Item->ProcessEvent(GetStackFunc, &Params);
			Stack = Params.ReturnValue;
		}
		SpawnItemInWorld(Item, Stack);
		InventoryList.Entries.RemoveAt(Idx);
		InventoryList.MarkArrayDirty();
	}
}

bool UOWRPGInventoryManagerComponent::ServerSplitStack_Validate(ULyraInventoryItemInstance* Item, int32 AmountToSplit) { return true; }
void UOWRPGInventoryManagerComponent::ServerSplitStack_Implementation(ULyraInventoryItemInstance* Item, int32 AmountToSplit)
{
	if (!Item || CursorItem || AmountToSplit <= 0) return;

	int32 CurrentStack = 0;
	static UFunction* GetStackFunc = Item->FindFunction(TEXT("GetStatTagStackCount"));
	if (GetStackFunc)
	{
		struct FParams { FGameplayTag Tag; int32 ReturnValue; };
		FParams Params = { OWRPGGameplayTags::OWRPG_Inventory_Stack, 0 };
		Item->ProcessEvent(GetStackFunc, &Params);
		CurrentStack = Params.ReturnValue;
	}

	if (CurrentStack <= AmountToSplit) return;

	static UFunction* RemoveStackFunc = Item->FindFunction(TEXT("RemoveStatTagStack"));
	if (RemoveStackFunc)
	{
		struct FParams { FGameplayTag Tag; int32 Count; };
		FParams Params = { OWRPGGameplayTags::OWRPG_Inventory_Stack, AmountToSplit };
		Item->ProcessEvent(RemoveStackFunc, &Params);
	}

	ULyraInventoryItemInstance* NewItem = NewObject<ULyraInventoryItemInstance>(GetOwner());

	static FProperty* ItemDefProp = FindFProperty<FProperty>(ULyraInventoryItemInstance::StaticClass(), TEXT("ItemDef"));
	if (ItemDefProp)
	{
		TSubclassOf<ULyraInventoryItemDefinition> Def = Item->GetItemDef();
		UClass* DefClass = *Def;
		if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(ItemDefProp))
		{
			ObjProp->SetObjectPropertyValue_InContainer(NewItem, DefClass);
		}
	}

	static UFunction* AddStackFunc = NewItem->FindFunction(TEXT("AddStatTagStack"));
	if (AddStackFunc)
	{
		struct FParams { FGameplayTag Tag; int32 Count; };
		FParams Params = { OWRPGGameplayTags::OWRPG_Inventory_Stack, AmountToSplit };
		NewItem->ProcessEvent(AddStackFunc, &Params);
	}

	CursorItem = NewItem;
	InventoryList.MarkArrayDirty();
}

bool UOWRPGInventoryManagerComponent::ServerEquipItem_Validate(ULyraInventoryItemInstance* Item) { return true; }
void UOWRPGInventoryManagerComponent::ServerEquipItem_Implementation(ULyraInventoryItemInstance* Item)
{
	Internal_RemoveItem(Item);
	UE_LOG(LogTemp, Warning, TEXT("Server: Item Equipped"));
}

float UOWRPGInventoryManagerComponent::GetTotalWeight() const { return 0.0f; }

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
			// Spawn in front of player
			FVector SpawnLoc = Pawn->GetActorLocation() + (Pawn->GetActorForwardVector() * 100.0f) + FVector(0, 0, 50.0f);
			FRotator RandomRot = Pawn->GetActorRotation();
			RandomRot.Yaw += FMath::RandRange(-20.0f, 20.0f);

			FTransform SpawnTransform(RandomRot, SpawnLoc);

			if (AOWRPGWorldCollectable* NewPickup = GetWorld()->SpawnActorDeferred<AOWRPGWorldCollectable>(PickupFrag->PickupActorClass, SpawnTransform, Pawn, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
			{
				NewPickup->StaticItemDefinition = Item->GetItemDef();
				NewPickup->StackCount = StackCount;
				NewPickup->FinishSpawning(SpawnTransform);
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Server: Spawned Item %s x%d"), *GetNameSafe(Item->GetItemDef()), StackCount);
}