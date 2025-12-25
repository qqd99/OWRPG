// Copyright Legion. All Rights Reserved.

#include "AbilitySystem/OWRPGAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_OWRPG_InitStats.h"
#include "System/OWRPGGameplayTags.h"

void UOWRPGAbilitySystemLibrary::InitializeRandomStats(UAbilitySystemComponent* ASC, FGameplayTag CharacterRace)
{
	if (!ASC) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ASC->GetAvatarActor());
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UGE_OWRPG_InitStats::StaticClass(), 1.0f, Context);

	if (SpecHandle.IsValid())
	{
		FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

		// --- 1. Roll Primary Stats ---
		// We use a Map to store them temporarily so we can use them for math later (like Luck -> Crit)
		TMap<FGameplayTag, float> StatValues;

		auto Roll = [&](FGameplayTag Tag, float Base, float Variance)
			{
				float Val = FMath::RoundToFloat(Base + FMath::RandRange(-Variance, Variance));
				StatValues.Add(Tag, Val);
				Spec->SetSetByCallerMagnitude(Tag, Val);
			};

		// Default Human Rolls
		float BaseStat = 10.0f;
		float Var = 3.0f;

		// Race Overrides
		if (CharacterRace == OWRPGGameplayTags::Race_RockPerson)
		{
			// Rock people are strong but slow
			Roll(OWRPGGameplayTags::Attribute_Strength, 15.0f, 2.0f);
			Roll(OWRPGGameplayTags::Attribute_Agility, 5.0f, 2.0f);
		}
		else
		{
			Roll(OWRPGGameplayTags::Attribute_Strength, BaseStat, Var);
			Roll(OWRPGGameplayTags::Attribute_Agility, BaseStat, Var);
		}

		Roll(OWRPGGameplayTags::Attribute_Intelligence, BaseStat, Var);
		Roll(OWRPGGameplayTags::Attribute_Endurance, BaseStat, Var);
		Roll(OWRPGGameplayTags::Attribute_Luck, BaseStat, Var); // Important for Crit
		Roll(OWRPGGameplayTags::Attribute_Willpower, BaseStat, Var);

		// --- 2. Calculate Combat Stats (Logic) ---

		// Defense Logic
		float Defense = 0.0f;
		if (CharacterRace == OWRPGGameplayTags::Race_RockPerson)
		{
			Defense = 10.0f; // Rock skin
		}
		Spec->SetSetByCallerMagnitude(OWRPGGameplayTags::Attribute_Defense, Defense);

		// Critical Chance Logic
		// "Normal people get 1, high luck get 10"
		float Luck = StatValues[OWRPGGameplayTags::Attribute_Luck];
		float CritChance = 1.0f;
		if (Luck >= 15.0f) // If very lucky
		{
			CritChance = 10.0f;
		}
		Spec->SetSetByCallerMagnitude(OWRPGGameplayTags::Attribute_CriticalChance, CritChance);

		// --- 3. Survival Defaults ---
		Spec->SetSetByCallerMagnitude(OWRPGGameplayTags::Attribute_MaxStamina, 100.0f);
		Spec->SetSetByCallerMagnitude(OWRPGGameplayTags::Attribute_MaxMana, 100.0f);
		Spec->SetSetByCallerMagnitude(OWRPGGameplayTags::Attribute_MaxHunger, 100.0f);
		Spec->SetSetByCallerMagnitude(OWRPGGameplayTags::Attribute_MaxThirst, 100.0f);

		ASC->ApplyGameplayEffectSpecToSelf(*Spec);
	}
}