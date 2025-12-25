// Copyright Legion. All Rights Reserved.

#include "Inventory/GA_EquipItem.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/InventoryFragment_EquippableItem.h"
#include "Inventory/OWRPGInventoryFunctionLibrary.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Equipment/LyraEquipmentDefinition.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Equipment/LyraGameplayAbility_FromEquipment.h"
#include "Character/LyraCharacter.h" 
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NativeGameplayTags.h"
#include "UObject/UObjectGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_EquipItem)

UGA_EquipItem::UGA_EquipItem()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_EquipItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	// We want the Client to SEND the prediction key, but stop before changing game state.
	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ULyraInventoryItemInstance* ItemInstance = nullptr;

	if (TriggerEventData && TriggerEventData->OptionalObject)
	{
		ItemInstance = const_cast<ULyraInventoryItemInstance*>(Cast<ULyraInventoryItemInstance>(TriggerEventData->OptionalObject));
	}

	if (ItemInstance)
	{
		EquipItem(ItemInstance);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_EquipItem::EquipItem(ULyraInventoryItemInstance* ItemInstance)
{
	ALyraCharacter* LyraChar = GetLyraCharacterFromActorInfo();
	if (!LyraChar)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ULyraEquipmentManagerComponent* EquipmentComponent = LyraChar->FindComponentByClass<ULyraEquipmentManagerComponent>();
	if (EquipmentComponent && ItemInstance)
	{
		const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(ItemInstance->GetItemDef());

		// Use the centralized helper from our Function Library
		if (const UInventoryFragment_EquippableItem* EquipFragment = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UInventoryFragment_EquippableItem>(Def))
		{
			if (EquipFragment->EquipmentDefinition)
			{
				// Capture the new Equipment Instance
				ULyraEquipmentInstance* NewEquip = EquipmentComponent->EquipItem(EquipFragment->EquipmentDefinition);
				
				// Manually link it to the Inventory Item!
				if (NewEquip)
				{
					NewEquip->SetInstigator(ItemInstance);

					// Update visuals for generic items
					for (AActor* SpawnedActor : NewEquip->GetSpawnedActors())
					{
						UOWRPGInventoryFunctionLibrary::UpdateGenericItemVisuals(SpawnedActor, ItemInstance);
					}
				}
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}