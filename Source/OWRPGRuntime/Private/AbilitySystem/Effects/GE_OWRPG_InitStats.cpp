// Copyright Legion. All Rights Reserved.

#include "AbilitySystem/Effects/GE_OWRPG_InitStats.h"
#include "AbilitySystem/Attributes/OWRPGBaseStatSet.h"
#include "AbilitySystem/Attributes/OWRPGStaminaSet.h"
#include "AbilitySystem/Attributes/OWRPGManaSet.h"
#include "System/OWRPGGameplayTags.h"

UGE_OWRPG_InitStats::UGE_OWRPG_InitStats()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	auto AddAttributeModifier = [&](FGameplayAttribute Attribute, FGameplayTag SetByCallerTag)
		{
			FGameplayModifierInfo Modifier;
			Modifier.Attribute = Attribute;
			Modifier.ModifierOp = EGameplayModOp::Override;
			FSetByCallerFloat SetByCaller;
			SetByCaller.DataTag = SetByCallerTag;
			Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
			Modifiers.Add(Modifier);
		};

	// Primary
	AddAttributeModifier(UOWRPGBaseStatSet::GetStrengthAttribute(), OWRPGGameplayTags::Attribute_Strength);
	AddAttributeModifier(UOWRPGBaseStatSet::GetAgilityAttribute(), OWRPGGameplayTags::Attribute_Agility);
	AddAttributeModifier(UOWRPGBaseStatSet::GetIntelligenceAttribute(), OWRPGGameplayTags::Attribute_Intelligence);
	AddAttributeModifier(UOWRPGBaseStatSet::GetEnduranceAttribute(), OWRPGGameplayTags::Attribute_Endurance);
	AddAttributeModifier(UOWRPGBaseStatSet::GetLuckAttribute(), OWRPGGameplayTags::Attribute_Luck);
	AddAttributeModifier(UOWRPGBaseStatSet::GetWillpowerAttribute(), OWRPGGameplayTags::Attribute_Willpower);

	// Combat
	AddAttributeModifier(UOWRPGBaseStatSet::GetDefenseAttribute(), OWRPGGameplayTags::Attribute_Defense);
	AddAttributeModifier(UOWRPGBaseStatSet::GetCriticalChanceAttribute(), OWRPGGameplayTags::Attribute_CriticalChance);

	// Survival
	AddAttributeModifier(UOWRPGStaminaSet::GetMaxStaminaAttribute(), OWRPGGameplayTags::Attribute_MaxStamina);
	AddAttributeModifier(UOWRPGManaSet::GetMaxManaAttribute(), OWRPGGameplayTags::Attribute_MaxMana);
	AddAttributeModifier(UOWRPGBaseStatSet::GetMaxHungerAttribute(), OWRPGGameplayTags::Attribute_MaxHunger);
	AddAttributeModifier(UOWRPGBaseStatSet::GetMaxThirstAttribute(), OWRPGGameplayTags::Attribute_MaxThirst);
}