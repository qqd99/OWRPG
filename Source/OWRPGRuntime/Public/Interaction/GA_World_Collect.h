// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_World_Collect.generated.h"

/**
 * Ability to collect an item from the world.
 */
UCLASS()
class OWRPGRUNTIME_API UGA_World_Collect : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_World_Collect();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};