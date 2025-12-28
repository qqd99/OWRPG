// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Inventory/LyraInventoryItemDefinition.h" 
#include "OWRPGInventoryFunctionLibrary.generated.h"

class ULyraInventoryItemInstance;
class UOWRPGInventoryFragment_CoreStats;
class AController;
class ULyraEquipmentInstance;

/**
 * Helper library for OWRPG Inventory logic.
 */
UCLASS()
class OWRPGRUNTIME_API UOWRPGInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// --- HELPER TO BYPASS LINKER ERRORS ---

	// Template Version (C++ Only)
	template <typename T>
	static const T* FindItemDefinitionFragment(const ULyraInventoryItemDefinition* ItemDef)
	{
		if ((ItemDef != nullptr) && (ItemDef->Fragments.Num() > 0))
		{
			for (const ULyraInventoryItemFragment* Fragment : ItemDef->Fragments)
			{
				if (const T* TypedFrag = Cast<T>(Fragment))
				{
					return TypedFrag;
				}
			}
		}
		return nullptr;
	}

	// Blueprint Version
	UFUNCTION(BlueprintCallable, Category = "OWRPG|Inventory", meta = (DeterminesOutputType = "FragmentClass"))
	static const ULyraInventoryItemFragment* FindItemDefinitionFragment(const ULyraInventoryItemDefinition* ItemDef, TSubclassOf<ULyraInventoryItemFragment> FragmentClass);

	// --------------------------------------

	// --- STACKING HELPERS (Reflected to avoid Linker Errors) ---
	// These replace direct calls to LyraInventoryItemInstance::AddStatTagStack to fix LNK2019

	UFUNCTION(BlueprintCallable, Category = "OWRPG|Inventory|Stacking")
	static int32 GetItemStatsStackCount(ULyraInventoryItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category = "OWRPG|Inventory|Stacking")
	static bool HasItemStatsStack(ULyraInventoryItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category = "OWRPG|Inventory|Stacking")
	static void AddItemStatsStack(ULyraInventoryItemInstance* Item, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "OWRPG|Inventory|Stacking")
	static void RemoveItemStatsStack(ULyraInventoryItemInstance* Item, int32 Count);

	// --------------------------------------

	// --- TRAIT & CATEGORY SYSTEM ---

	/** Checks if an item definition has a specific trait (e.g., Trait.Material.Metal). */
	UFUNCTION(BlueprintCallable, Category = "OWRPG|Inventory", meta = (DeterminesOutputType = "FragmentClass"))
	static bool HasTrait(const TSubclassOf<ULyraInventoryItemDefinition> ItemDef, FGameplayTag TraitTag, bool bExact = false);

	/** Checks if an Item Instance has a trait. */
	UFUNCTION(BlueprintCallable, Category = "OWRPG|Inventory")
	static bool InstanceHasTrait(const ULyraInventoryItemInstance* ItemInstance, FGameplayTag TraitTag, bool bExact = false);

	/** Returns the UI Category for an item (Weapon, Consumable, etc.). */
	UFUNCTION(BlueprintCallable, Category = "OWRPG|Inventory")
	static FGameplayTag GetItemCategory(const ULyraInventoryItemInstance* ItemInstance);


	// --- CORE STATS & UTILITY ---

	/** Returns the CoreStats fragment from an item instance (if it exists). */
	UFUNCTION(BlueprintPure, Category = "OWRPG|Inventory")
	static const UOWRPGInventoryFragment_CoreStats* GetItemStats(const ULyraInventoryItemInstance* ItemInstance);

	/** Calculates the total weight of an item stack (Weight * Count). */
	UFUNCTION(BlueprintPure, Category = "OWRPG|Inventory")
	static float GetItemWeight(const ULyraInventoryItemInstance* ItemInstance);

	/** Returns the max stack limit for this item. */
	UFUNCTION(BlueprintPure, Category = "OWRPG|Inventory")
	static int32 GetItemMaxStack(const ULyraInventoryItemInstance* ItemInstance);

	/** Helper to get the Display Name (Lyra default fragment). */
	UFUNCTION(BlueprintPure, Category = "OWRPG|Inventory")
	static FText GetItemDisplayName(const ULyraInventoryItemInstance* ItemInstance);

	/** Gets the current stack count (Quantity) of the item instance. */
	UFUNCTION(BlueprintPure, Category = "OWRPG|Inventory")
	static int32 GetItemQuantity(const ULyraInventoryItemInstance* ItemInstance);

	/** Returns all valid item instances from a controller's inventory manager. */
	UFUNCTION(BlueprintCallable, Category = "OWRPG|Inventory")
	static TArray<ULyraInventoryItemInstance*> GetAllItems(AController* Controller);

	/** Removes a specific item instance from the controller's inventory manager. */
	UFUNCTION(BlueprintCallable, Category = "OWRPG|Inventory")
	static bool RemoveItemFromInventory(AController* Controller, ULyraInventoryItemInstance* ItemInstance);


	// --- EQUIPMENT UTILITY ---

	/** Helper to find the Equipment Instance associated with a specific Inventory Item. */
	UFUNCTION(BlueprintCallable, Category = "OWRPG|Equipment")
	static ULyraEquipmentInstance* FindEquipmentByItem(AController* Controller, ULyraInventoryItemInstance* ItemInstance);

	/** Unequips the specific equipment instance. */
	UFUNCTION(BlueprintCallable, Category = "OWRPG|Equipment")
	static void UnequipItem(AController* Controller, ULyraEquipmentInstance* Equipment);

	static void UpdateGenericItemVisuals(AActor* VisualActor, ULyraInventoryItemInstance* Item);

	// --- UI HELPERS ---

	/** Gets the Icon from the UI Fragment. Returns nullptr if missing. */
	UFUNCTION(BlueprintPure, Category = "OWRPG|Inventory")
	static UTexture2D* GetItemIcon(const ULyraInventoryItemInstance* ItemInstance);

	/** Gets the Description from the UI Fragment. */
	UFUNCTION(BlueprintPure, Category = "OWRPG|Inventory")
	static FText GetItemDescription(const ULyraInventoryItemInstance* ItemInstance);
};