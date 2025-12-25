// Copyright Legion. All Rights Reserved.

#include "Inventory/GA_DropItem.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "Inventory/OWRPGInventoryFunctionLibrary.h"
#include "Inventory/OWRPGInventoryFragment_Pickup.h"
#include "Interaction/OWRPGWorldCollectable.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Player/LyraPlayerController.h" 
#include "GameFramework/Actor.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_DropItem)

UGA_DropItem::UGA_DropItem()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_DropItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	// --- AUTHORITY CHECK ---
	// GAS Prediction Rule:
	// If LocalPredicted, both Client and Server run this function.
	// We MUST ensure only the Server spawns the actor, or you get "Ghosts".
	if (!HasAuthority(&ActivationInfo))
	{
		// We are the Client. We have successfully triggered the ability, 
		// which sends the Prediction Key to the Server. Our job is done.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	// -----------------------

	// 1. Get the Item Instance
	ULyraInventoryItemInstance* ItemInstance = nullptr;
	if (TriggerEventData && TriggerEventData->OptionalObject)
	{
		ItemInstance = const_cast<ULyraInventoryItemInstance*>(Cast<ULyraInventoryItemInstance>(TriggerEventData->OptionalObject));
	}

	if (!ItemInstance)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. Get Controller
	ALyraPlayerController* PC = GetLyraPlayerControllerFromActorInfo();
	if (!PC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Check if this item is currently equipped
	if (ULyraEquipmentInstance* EquipInst = UOWRPGInventoryFunctionLibrary::FindEquipmentByItem(PC, ItemInstance))
	{
		// If found, unequip it immediately (Destroys the visual mesh in hand)
		UOWRPGInventoryFunctionLibrary::UnequipItem(PC, EquipInst);
	}

	ULyraInventoryManagerComponent* InventoryComp = PC->GetComponentByClass<ULyraInventoryManagerComponent>();
	APawn* Pawn = PC->GetPawn();

	if (InventoryComp && Pawn)
	{
		const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(ItemInstance->GetItemDef());

		if (const UOWRPGInventoryFragment_Pickup* PickupFrag = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UOWRPGInventoryFragment_Pickup>(Def))
		{
			if (PickupFrag->PickupActorClass)
			{
				// 3. Calculate Spawn Location
				// Change the forward vector multiplier or add Z height
				FVector SpawnLoc = Pawn->GetActorLocation() + (Pawn->GetActorForwardVector() * 100.0f) + FVector(0, 0, 50.0f);
				FRotator RandomRot = Pawn->GetActorRotation();
				RandomRot.Yaw += FMath::RandRange(-20.0f, 20.0f);

				FTransform SpawnTransform(RandomRot, SpawnLoc);

				// 4. Spawn the World Actor (SERVER ONLY due to check above)
				if (AOWRPGWorldCollectable* NewPickup = GetWorld()->SpawnActorDeferred<AOWRPGWorldCollectable>(PickupFrag->PickupActorClass, SpawnTransform, Pawn, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
				{
					NewPickup->StaticItemDefinition = ItemInstance->GetItemDef();
					NewPickup->StackCount = UOWRPGInventoryFunctionLibrary::GetItemQuantity(ItemInstance);

					NewPickup->FinishSpawning(SpawnTransform);

					// 5. Remove from Inventory
					InventoryComp->RemoveItemInstance(ItemInstance);
				}
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}