// Copyright Legion. All Rights Reserved.

#include "AbilitySystem/Attributes/OWRPGStaminaSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "System/OWRPGGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWRPGStaminaSet)

UOWRPGStaminaSet::UOWRPGStaminaSet()
	: Stamina(100.0f)
	, MaxStamina(100.0f)
	, StaminaRegenRate(0.0f)
	, bOutOfStamina(false)
{
}

void UOWRPGStaminaSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGStaminaSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGStaminaSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGStaminaSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
}

void UOWRPGStaminaSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGStaminaSet, Stamina, OldValue);
}

void UOWRPGStaminaSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGStaminaSet, MaxStamina, OldValue);
}

void UOWRPGStaminaSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGStaminaSet, StaminaRegenRate, OldValue);
}

bool UOWRPGStaminaSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void UOWRPGStaminaSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	float Magnitude = Data.EvaluatedData.Magnitude;
	float OldValue = GetStamina() - Magnitude;
	float NewValue = GetStamina();

	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		const bool bIsStaminaEmpty = (GetStamina() <= 0.0f);
		if (bIsStaminaEmpty != bOutOfStamina)
		{
			bOutOfStamina = bIsStaminaEmpty;
			FGameplayTag Tag = OWRPGGameplayTags::Status_Draining_Stamina;

			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				if (bOutOfStamina)
				{
					ASC->AddLooseGameplayTag(Tag);
				}
				else
				{
					ASC->RemoveLooseGameplayTag(Tag);
				}
			}
		}

		if (OnStaminaChanged.IsBound())
		{
			const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetContext();
			OnStaminaChanged.Broadcast(EffectContext.GetOriginalInstigator(), EffectContext.GetEffectCauser(), &Data.EffectSpec, Magnitude, OldValue, NewValue);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetMaxStaminaAttribute())
	{
		if (OnMaxStaminaChanged.IsBound())
		{
			const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetContext();
			OnMaxStaminaChanged.Broadcast(EffectContext.GetOriginalInstigator(), EffectContext.GetEffectCauser(), &Data.EffectSpec, Magnitude, OldValue, NewValue);
		}
	}
}

void UOWRPGStaminaSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UOWRPGStaminaSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UOWRPGStaminaSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxStaminaAttribute())
	{
		// Make sure current stamina is not greater than the new max stamina.
		if (GetStamina() > NewValue)
		{
			// Use ApplyModToAttributeUnsafe to enforce the limit safely
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (ASC)
			{
				ASC->ApplyModToAttributeUnsafe(GetStaminaAttribute(), EGameplayModOp::Override, NewValue);
			}
		}
	}
}

void UOWRPGStaminaSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetStaminaAttribute())
	{
		// Do not allow stamina to go negative or above max stamina.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		// Do not allow max stamina to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}