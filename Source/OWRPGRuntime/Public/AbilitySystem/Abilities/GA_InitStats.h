// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_InitStats.generated.h"

/**
 * Ability that runs once on spawn to roll random stats.
 */
UCLASS()
class OWRPGRUNTIME_API UGA_InitStats : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_InitStats();

	// Defines which Race to use for the roll.
	// In the future, this could be read from the PlayerState or HeroData.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OWRPG")
	FGameplayTag CharacterRace;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
};