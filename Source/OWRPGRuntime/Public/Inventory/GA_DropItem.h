// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_DropItem.generated.h"

class ULyraInventoryItemInstance;

/**
 * Ability to drop an item from inventory into the world.
 */
UCLASS()
class OWRPGRUNTIME_API UGA_DropItem : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_DropItem();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};