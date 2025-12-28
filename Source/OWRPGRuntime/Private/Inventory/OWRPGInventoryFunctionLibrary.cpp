// Copyright Legion. All Rights Reserved.

#include "Inventory/OWRPGInventoryFunctionLibrary.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Inventory/OWRPGInventoryFragment_Traits.h"
#include "Inventory/OWRPGInventoryFragment_CoreStats.h"
#include "Inventory/OWRPGInventoryFragment_UI.h" 
#include "GameFramework/Controller.h"
#include "System/GameplayTagStack.h" 
#include "UObject/UObjectGlobals.h"
#include "Equipment/OWRPGVisualItemActor.h"
#include "System/OWRPGGameplayTags.h"

// --- FRAGMENT HELPERS ---

const ULyraInventoryItemFragment* UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment(const ULyraInventoryItemDefinition* ItemDef, TSubclassOf<ULyraInventoryItemFragment> FragmentClass)
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		for (const ULyraInventoryItemFragment* Fragment : ItemDef->Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}
	return nullptr;
}

// --- STACKING REFLECTION HELPERS (Fix for LNK2019) ---

int32 UOWRPGInventoryFunctionLibrary::GetItemStatsStackCount(ULyraInventoryItemInstance* Item)
{
	if (!Item) return 0;

	// Use Reflection to find the function, since we can't link to it directly
	static UFunction* Func = Item->FindFunction(FName("GetStatTagStackCount"));
	if (Func)
	{
		struct FParams { FGameplayTag Tag; int32 ReturnValue; };
		FParams Params;
		Params.Tag = OWRPGGameplayTags::OWRPG_Inventory_Stack;
		Params.ReturnValue = 0;

		Item->ProcessEvent(Func, &Params);
		return Params.ReturnValue;
	}

	// Fallback: If stack system isn't on item, return 1
	return 1;
}

bool UOWRPGInventoryFunctionLibrary::HasItemStatsStack(ULyraInventoryItemInstance* Item)
{
	if (!Item) return false;

	static UFunction* Func = Item->FindFunction(FName("HasStatTag"));
	if (Func)
	{
		struct FParams { FGameplayTag Tag; bool ReturnValue; };
		FParams Params;
		Params.Tag = OWRPGGameplayTags::OWRPG_Inventory_Stack;
		Params.ReturnValue = false;

		Item->ProcessEvent(Func, &Params);
		return Params.ReturnValue;
	}
	return false;
}

void UOWRPGInventoryFunctionLibrary::AddItemStatsStack(ULyraInventoryItemInstance* Item, int32 Count)
{
	if (!Item || Count <= 0) return;

	static UFunction* Func = Item->FindFunction(FName("AddStatTagStack"));
	if (Func)
	{
		struct FParams { FGameplayTag Tag; int32 StackCount; };
		FParams Params;
		Params.Tag = OWRPGGameplayTags::OWRPG_Inventory_Stack;
		Params.StackCount = Count;

		Item->ProcessEvent(Func, &Params);
	}
}

void UOWRPGInventoryFunctionLibrary::RemoveItemStatsStack(ULyraInventoryItemInstance* Item, int32 Count)
{
	if (!Item || Count <= 0) return;

	static UFunction* Func = Item->FindFunction(FName("RemoveStatTagStack"));
	if (Func)
	{
		struct FParams { FGameplayTag Tag; int32 StackCount; };
		FParams Params;
		Params.Tag = OWRPGGameplayTags::OWRPG_Inventory_Stack;
		Params.StackCount = Count;

		Item->ProcessEvent(Func, &Params);
	}
}

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

int32 UOWRPGInventoryFunctionLibrary::GetItemQuantity(const ULyraInventoryItemInstance* ItemInstance)
{
	// Updated to use the new Helper
	return GetItemStatsStackCount(const_cast<ULyraInventoryItemInstance*>(ItemInstance));
}

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
			if (!Def->DisplayName.IsEmpty())
			{
				return Def->DisplayName;
			}
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

	if (AOWRPGVisualItemActor* SmartVisual = Cast<AOWRPGVisualItemActor>(VisualActor))
	{
		const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(Item->GetItemDef());
		if (const UOWRPGInventoryFragment_UI* UIFrag = FindItemDefinitionFragment<UOWRPGInventoryFragment_UI>(Def))
		{
			if (UIFrag->WorldMesh)
			{
				SmartVisual->SetItemMesh(UIFrag->WorldMesh);
			}
		}
	}
	else
	{
		if (UStaticMeshComponent* MeshComp = VisualActor->FindComponentByClass<UStaticMeshComponent>())
		{
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