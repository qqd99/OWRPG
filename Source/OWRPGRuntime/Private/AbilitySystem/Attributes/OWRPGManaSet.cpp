// Copyright Legion. All Rights Reserved.

#include "AbilitySystem/Attributes/OWRPGManaSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "System/OWRPGGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWRPGManaSet)

UOWRPGManaSet::UOWRPGManaSet()
	: Mana(100.0f)
	, MaxMana(100.0f)
	, ManaRegenRate(0.0f)
	, bOutOfMana(false)
{
}

void UOWRPGManaSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGManaSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGManaSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGManaSet, ManaRegenRate, COND_None, REPNOTIFY_Always);
}

void UOWRPGManaSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGManaSet, Mana, OldValue);
}

void UOWRPGManaSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGManaSet, MaxMana, OldValue);
}

void UOWRPGManaSet::OnRep_ManaRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGManaSet, ManaRegenRate, OldValue);
}

bool UOWRPGManaSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void UOWRPGManaSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	float Magnitude = Data.EvaluatedData.Magnitude;
	float OldValue = GetMana() - Magnitude; // Approximate old value
	float NewValue = GetMana();

	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		const bool bIsManaEmpty = (GetMana() <= 0.0f);
		if (bIsManaEmpty != bOutOfMana)
		{
			bOutOfMana = bIsManaEmpty;
			FGameplayTag Tag = OWRPGGameplayTags::Status_Draining_Mana;

			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				if (bOutOfMana)
				{
					ASC->AddLooseGameplayTag(Tag);
				}
				else
				{
					ASC->RemoveLooseGameplayTag(Tag);
				}
			}
		}

		// Broadcast change to listeners (UI, etc)
		if (OnManaChanged.IsBound())
		{
			const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetContext();
			OnManaChanged.Broadcast(EffectContext.GetOriginalInstigator(), EffectContext.GetEffectCauser(), &Data.EffectSpec, Magnitude, OldValue, NewValue);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetMaxManaAttribute())
	{
		if (OnMaxManaChanged.IsBound())
		{
			const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetContext();
			OnMaxManaChanged.Broadcast(EffectContext.GetOriginalInstigator(), EffectContext.GetEffectCauser(), &Data.EffectSpec, Magnitude, OldValue, NewValue);
		}
	}
}

void UOWRPGManaSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UOWRPGManaSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UOWRPGManaSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxManaAttribute())
	{
		// Make sure current mana is not greater than the new max mana.
		if (GetMana() > NewValue)
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (ASC)
			{
				ASC->ApplyModToAttributeUnsafe(GetManaAttribute(), EGameplayModOp::Override, NewValue);
			}
		}
	}
}

void UOWRPGManaSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetManaAttribute())
	{
		// Do not allow mana to go negative or above max mana.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		// Do not allow max mana to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}