// Copyright Legion. All Rights Reserved.

#include "Inventory/OWRPGInventoryFunctionLibrary.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Inventory/OWRPGInventoryFragment_Traits.h"
#include "Inventory/OWRPGInventoryFragment_CoreStats.h"
#include "Inventory/OWRPGInventoryFragment_UI.h" // Added for Icon/Description
#include "GameFramework/Controller.h"
#include "System/GameplayTagStack.h" // Required for Reflection cast
#include "UObject/UObjectGlobals.h"
#include "Equipment/OWRPGVisualItemActor.h"

// --- TRAIT LOGIC ---

bool UOWRPGInventoryFunctionLibrary::HasTrait(const TSubclassOf<ULyraInventoryItemDefinition> ItemDef, FGameplayTag TraitTag, bool bExact)
{
	if (!ItemDef || !TraitTag.IsValid()) return false;

	if (const ULyraInventoryItemDefinition* DefaultItem = GetDefault<ULyraInventoryItemDefinition>(ItemDef))
	{
		if (const UOWRPGInventoryFragment_Traits* TraitFragment = FindItemDefinitionFragment<UOWRPGInventoryFragment_Traits>(DefaultItem))
		{
			return bExact ? TraitFragment->Traits.HasTagExact(TraitTag) : TraitFragment->Traits.HasTag(TraitTag);
		}
	}
	return false;
}

bool UOWRPGInventoryFunctionLibrary::InstanceHasTrait(const ULyraInventoryItemInstance* ItemInstance, FGameplayTag TraitTag, bool bExact)
{
	if (!ItemInstance) return false;

	const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(ItemInstance->GetItemDef());
	if (!Def) return false;

	if (const UOWRPGInventoryFragment_Traits* TraitFragment = FindItemDefinitionFragment<UOWRPGInventoryFragment_Traits>(Def))
	{
		return bExact ? TraitFragment->Traits.HasTagExact(TraitTag) : TraitFragment->Traits.HasTag(TraitTag);
	}
	return false;
}

FGameplayTag UOWRPGInventoryFunctionLibrary::GetItemCategory(const ULyraInventoryItemInstance* ItemInstance)
{
	if (ItemInstance)
	{
		if (const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(ItemInstance->GetItemDef()))
		{
			if (const UOWRPGInventoryFragment_Traits* TraitFragment = FindItemDefinitionFragment<UOWRPGInventoryFragment_Traits>(Def))
			{
				return TraitFragment->ItemCategory;
			}
		}
	}
	return FGameplayTag();
}

// --- CORE STATS & UTILITY ---

const UOWRPGInventoryFragment_CoreStats* UOWRPGInventoryFunctionLibrary::GetItemStats(const ULyraInventoryItemInstance* ItemInstance)
{
	if (ItemInstance)
	{
		if (const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(ItemInstance->GetItemDef()))
		{
			return FindItemDefinitionFragment<UOWRPGInventoryFragment_CoreStats>(Def);
		}
	}
	return nullptr;
}

// 1. Centralized Stack Count Logic (The Reflection Fix)
int32 UOWRPGInventoryFunctionLibrary::GetItemQuantity(const ULyraInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance) return 0;

	// Default to 1 (If instance exists, there is at least 1)
	int32 Count = 1;

	// WORKAROUND: LyraInventoryItemInstance::GetStatTagStackCount is not exported.
	// We use reflection to find the private 'StatTags' property.
	static FProperty* StatTagsProp = ULyraInventoryItemInstance::StaticClass()->FindPropertyByName(FName("StatTags"));

	if (FStructProperty* StructProp = CastField<FStructProperty>(StatTagsProp))
	{
		if (StructProp->Struct->GetFName() == FName("GameplayTagStackContainer"))
		{
			const FGameplayTagStackContainer* Container = StructProp->ContainerPtrToValuePtr<FGameplayTagStackContainer>(ItemInstance);
			if (Container)
			{
				// Check for the standard Lyra stack tag. 
				// 'false' prevents error log if tag is missing in config.
				FGameplayTag StackTag = FGameplayTag::RequestGameplayTag(FName("Lyra.Inventory.StackCount"), false);

				if (StackTag.IsValid())
				{
					int32 FoundCount = Container->GetStackCount(StackTag);
					if (FoundCount > 0)
					{
						Count = FoundCount;
					}
				}
			}
		}
	}
	return Count;
}

// 2. Updated Weight Function (Uses GetItemQuantity)
float UOWRPGInventoryFunctionLibrary::GetItemWeight(const ULyraInventoryItemInstance* ItemInstance)
{
	if (const UOWRPGInventoryFragment_CoreStats* Stats = GetItemStats(ItemInstance))
	{
		int32 Count = GetItemQuantity(ItemInstance);
		return Stats->Weight * (float)Count;
	}
	return 0.0f;
}

int32 UOWRPGInventoryFunctionLibrary::GetItemMaxStack(const ULyraInventoryItemInstance* ItemInstance)
{
	if (const UOWRPGInventoryFragment_CoreStats* Stats = GetItemStats(ItemInstance))
	{
		return Stats->MaxStack;
	}
	return 1;
}

// --- VISUALS (UI) ---

FText UOWRPGInventoryFunctionLibrary::GetItemDisplayName(const ULyraInventoryItemInstance* ItemInstance)
{
	if (ItemInstance)
	{
		if (const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(ItemInstance->GetItemDef()))
		{
			// 1. Check Native Lyra Display Name
			if (!Def->DisplayName.IsEmpty())
			{
				return Def->DisplayName;
			}
			// 2. Fallback to Class Name
			return FText::FromString(Def->GetName());
		}
	}
	return FText::GetEmpty();
}

UTexture2D* UOWRPGInventoryFunctionLibrary::GetItemIcon(const ULyraInventoryItemInstance* ItemInstance)
{
	if (ItemInstance)
	{
		if (const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(ItemInstance->GetItemDef()))
		{
			if (const UOWRPGInventoryFragment_UI* UIFrag = FindItemDefinitionFragment<UOWRPGInventoryFragment_UI>(Def))
			{
				return UIFrag->Icon;
			}
		}
	}
	return nullptr;
}

FText UOWRPGInventoryFunctionLibrary::GetItemDescription(const ULyraInventoryItemInstance* ItemInstance)
{
	if (ItemInstance)
	{
		if (const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(ItemInstance->GetItemDef()))
		{
			if (const UOWRPGInventoryFragment_UI* UIFrag = FindItemDefinitionFragment<UOWRPGInventoryFragment_UI>(Def))
			{
				return UIFrag->Description;
			}
		}
	}
	return FText::GetEmpty();
}

TArray<ULyraInventoryItemInstance*> UOWRPGInventoryFunctionLibrary::GetAllItems(AController* Controller)
{
	TArray<ULyraInventoryItemInstance*> Results;
	if (Controller)
	{
		if (ULyraInventoryManagerComponent* InventoryComp = Controller->FindComponentByClass<ULyraInventoryManagerComponent>())
		{
			Results = InventoryComp->GetAllItems();
		}
	}
	return Results;
}

bool UOWRPGInventoryFunctionLibrary::RemoveItemFromInventory(AController* Controller, ULyraInventoryItemInstance* ItemInstance)
{
	if (Controller && ItemInstance)
	{
		if (ULyraInventoryManagerComponent* InventoryComp = Controller->FindComponentByClass<ULyraInventoryManagerComponent>())
		{
			InventoryComp->RemoveItemInstance(ItemInstance);
			return true;
		}
	}
	return false;
}

// --- EQUIPMENT UTILITY ---

ULyraEquipmentInstance* UOWRPGInventoryFunctionLibrary::FindEquipmentByItem(AController* Controller, ULyraInventoryItemInstance* ItemInstance)
{
	if (Controller && ItemInstance)
	{
		if (APawn* Pawn = Controller->GetPawn())
		{
			if (ULyraEquipmentManagerComponent* EquipmentComp = Pawn->FindComponentByClass<ULyraEquipmentManagerComponent>())
			{
				TArray<ULyraEquipmentInstance*> EquipmentList = EquipmentComp->GetEquipmentInstancesOfType(ULyraEquipmentInstance::StaticClass());
				for (ULyraEquipmentInstance* EquipInst : EquipmentList)
				{
					if (EquipInst->GetInstigator() == ItemInstance)
					{
						return EquipInst;
					}
				}
			}
		}
	}
	return nullptr;
}

void UOWRPGInventoryFunctionLibrary::UnequipItem(AController* Controller, ULyraEquipmentInstance* Equipment)
{
	if (Controller && Equipment)
	{
		if (APawn* Pawn = Controller->GetPawn())
		{
			if (ULyraEquipmentManagerComponent* EquipmentComp = Pawn->FindComponentByClass<ULyraEquipmentManagerComponent>())
			{
				EquipmentComp->UnequipItem(Equipment);
			}
		}
	}
}

void UOWRPGInventoryFunctionLibrary::UpdateGenericItemVisuals(AActor* VisualActor, ULyraInventoryItemInstance* Item)
{
	if (!VisualActor || !Item) return;

	// Cast to smart actor
	if (AOWRPGVisualItemActor* SmartVisual = Cast<AOWRPGVisualItemActor>(VisualActor))
	{
		const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(Item->GetItemDef());
		if (const UOWRPGInventoryFragment_UI* UIFrag = FindItemDefinitionFragment<UOWRPGInventoryFragment_UI>(Def))
		{
			if (UIFrag->WorldMesh)
			{
				// Call the function that handles replication
				SmartVisual->SetItemMesh(UIFrag->WorldMesh);
			}
		}
	}
	else
	{
		// Try to find a Static Mesh Component
		if (UStaticMeshComponent* MeshComp = VisualActor->FindComponentByClass<UStaticMeshComponent>())
		{
			// Get the mesh from the Item Definition -> UI Fragment
			const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(Item->GetItemDef());
			if (const UOWRPGInventoryFragment_UI* UIFrag = FindItemDefinitionFragment<UOWRPGInventoryFragment_UI>(Def))
			{
				if (UIFrag->WorldMesh)
				{
					MeshComp->SetStaticMesh(UIFrag->WorldMesh);
				}
			}
		}
	}
}