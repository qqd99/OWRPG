// Copyright Legion. All Rights Reserved.

#include "AbilitySystem/Abilities/GA_InitStats.h"
#include "AbilitySystem/OWRPGAbilitySystemLibrary.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "System/OWRPGGameplayTags.h"

UGA_InitStats::UGA_InitStats()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	CharacterRace = OWRPGGameplayTags::Race_Human;
}

void UGA_InitStats::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (!Spec.IsActive())
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

void UGA_InitStats::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (HasAuthority(&ActivationInfo))
	{
		if (UAbilitySystemComponent* ASC = GetLyraAbilitySystemComponentFromActorInfo())
		{
			UOWRPGAbilitySystemLibrary::InitializeRandomStats(ASC, CharacterRace);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}