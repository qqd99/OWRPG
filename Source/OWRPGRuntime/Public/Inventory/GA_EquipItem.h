// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_EquipItem.generated.h"

class ULyraInventoryItemInstance;

/**
 * Ability to equip an item from the Inventory.
 */
UCLASS()
class OWRPGRUNTIME_API UGA_EquipItem : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EquipItem();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	void EquipItem(ULyraInventoryItemInstance* ItemInstance);
};