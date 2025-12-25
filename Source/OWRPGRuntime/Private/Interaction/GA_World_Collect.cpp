// Copyright Legion. All Rights Reserved.

#include "Interaction/GA_World_Collect.h"
#include "Interaction/OWRPGWorldCollectable.h"
#include "Inventory/OWRPGInventoryManagerComponent.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Player/LyraPlayerController.h" 
#include "GameFramework/Actor.h"
#include "Abilities/GameplayAbilityTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_World_Collect)

UGA_World_Collect::UGA_World_Collect()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_World_Collect::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	// 1. Get the Lyra Player Controller
	ALyraPlayerController* PC = GetLyraPlayerControllerFromActorInfo();
	if (!PC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. Identify the Target
	AOWRPGWorldCollectable* Collectable = nullptr;
	if (TriggerEventData)
	{
		const AActor* TargetActor = TriggerEventData->Target.Get();
		Collectable = Cast<AOWRPGWorldCollectable>(const_cast<AActor*>(TargetActor));
	}

	if (!Collectable)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. Get the Inventory Component
	UOWRPGInventoryManagerComponent* InventoryComponent = PC->GetComponentByClass<UOWRPGInventoryManagerComponent>();

	if (InventoryComponent && Collectable->StaticItemDefinition)
	{
		// 4. Add the Item using Grid Logic
		// We use Page 0 (Backpack) by default.
		InventoryComponent->AddItemDefinition(Collectable->StaticItemDefinition, Collectable->StackCount);

		// 5. Destroy the World Actor
		Collectable->Destroy();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}