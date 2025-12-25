// Copyright Legion. All Rights Reserved.

#include "AbilitySystem/Attributes/OWRPGBaseStatSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "System/OWRPGGameplayTags.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OWRPGBaseStatSet)

UOWRPGBaseStatSet::UOWRPGBaseStatSet()
	: Strength(10.0f)
	, Agility(10.0f)
	, Intelligence(10.0f)
	, Endurance(10.0f)
	, Luck(10.0f)
	, Willpower(10.0f)
	, Defense(0.0f)
	, CriticalChance(0.0f)
	, Hunger(100.0f)
	, MaxHunger(100.0f)
	, Thirst(100.0f)
	, MaxThirst(100.0f)
{
}

void UOWRPGBaseStatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Primary
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, Agility, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, Endurance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, Luck, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, Willpower, COND_None, REPNOTIFY_Always);

	// Combat
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, CriticalChance, COND_None, REPNOTIFY_Always);

	// Survival
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, Hunger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, MaxHunger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, Thirst, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOWRPGBaseStatSet, MaxThirst, COND_None, REPNOTIFY_Always);
}

// --- OnRep Functions ---

void UOWRPGBaseStatSet::OnRep_Strength(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, Strength, OldValue); }
void UOWRPGBaseStatSet::OnRep_Agility(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, Agility, OldValue); }
void UOWRPGBaseStatSet::OnRep_Intelligence(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, Intelligence, OldValue); }
void UOWRPGBaseStatSet::OnRep_Endurance(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, Endurance, OldValue); }
void UOWRPGBaseStatSet::OnRep_Luck(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, Luck, OldValue); }
void UOWRPGBaseStatSet::OnRep_Willpower(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, Willpower, OldValue); }

void UOWRPGBaseStatSet::OnRep_Defense(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, Defense, OldValue); }
void UOWRPGBaseStatSet::OnRep_CriticalChance(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, CriticalChance, OldValue); }

void UOWRPGBaseStatSet::OnRep_Hunger(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, Hunger, OldValue); }
void UOWRPGBaseStatSet::OnRep_MaxHunger(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, MaxHunger, OldValue); }
void UOWRPGBaseStatSet::OnRep_Thirst(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, Thirst, OldValue); }
void UOWRPGBaseStatSet::OnRep_MaxThirst(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UOWRPGBaseStatSet, MaxThirst, OldValue); }


void UOWRPGBaseStatSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// This is where you would trigger "Starvation Damage" if Hunger <= 0
	/*
	if (Data.EvaluatedData.Attribute == GetHungerAttribute())
	{
		if (GetHunger() <= 0.0f)
		{
			// Apply Starvation Tag or Damage Effect
		}
	}
	*/
}

void UOWRPGBaseStatSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UOWRPGBaseStatSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UOWRPGBaseStatSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	// Survival Clamps
	if (Attribute == GetHungerAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHunger());
	}
	else if (Attribute == GetThirstAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxThirst());
	}
	// Max Stat Clamps
	else if (Attribute == GetMaxHungerAttribute() || Attribute == GetMaxThirstAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	// Primary Stat Clamps (No negative strength)
	else if (Attribute == GetStrengthAttribute() || Attribute == GetAgilityAttribute() ||
		Attribute == GetIntelligenceAttribute() || Attribute == GetEnduranceAttribute() ||
		Attribute == GetLuckAttribute() || Attribute == GetWillpowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}