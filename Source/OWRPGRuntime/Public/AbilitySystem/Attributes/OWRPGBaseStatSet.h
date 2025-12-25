// Copyright Legion. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraAttributeSet.h"
#include "NativeGameplayTags.h"

#include "OWRPGBaseStatSet.generated.h"

/**
 * UOWRPGBaseStatSet
 *
 * The foundational stats for characters in the OWRPG.
 * Includes "S.P.E.C.I.A.L." style primary stats and Survival needs.
 */
UCLASS(BlueprintType)
class OWRPGRUNTIME_API UOWRPGBaseStatSet : public ULyraAttributeSet
{
	GENERATED_BODY()

public:

	UOWRPGBaseStatSet();

	// -------------------------------------------------------------------
	//	Primary Stats (The "Build" of your character)
	// -------------------------------------------------------------------

	// Physical Power (Melee Dmg, Carry Weight)
	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, Strength);

	// Reflexes & Speed (Attack Speed, Movement Speed)
	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, Agility);

	// Mental Acuity (Mana Pool, Spell Power)
	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, Intelligence);

	// Physical Resistance (Health Pool, Stamina Drain reduction)
	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, Endurance);

	// Fortune (Crit Chance, Loot Quality)
	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, Luck);

	// Mental Resistance (Mana Regen, Status Effect Resistance)
	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, Willpower);


	// -------------------------------------------------------------------
	//	Combat Stats (Often derived from Primary, but settable directly)
	// -------------------------------------------------------------------

	// Damage mitigation
	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, Defense);

	// Critical Hit Chance (0.0 to 1.0)
	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, CriticalChance);


	// -------------------------------------------------------------------
	//	Survival Stats (The "Sim" elements)
	// -------------------------------------------------------------------

	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, Hunger);
	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, MaxHunger);

	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, Thirst);
	ATTRIBUTE_ACCESSORS(UOWRPGBaseStatSet, MaxThirst);


	// -------------------------------------------------------------------
	//	Delegates (For UI)
	// -------------------------------------------------------------------

	mutable FLyraAttributeEvent OnStrengthChanged;
	mutable FLyraAttributeEvent OnAgilityChanged;
	mutable FLyraAttributeEvent OnIntelligenceChanged;
	mutable FLyraAttributeEvent OnEnduranceChanged;
	mutable FLyraAttributeEvent OnLuckChanged;
	mutable FLyraAttributeEvent OnWillpowerChanged;

	mutable FLyraAttributeEvent OnDefenseChanged;
	mutable FLyraAttributeEvent OnCriticalChanceChanged;

	mutable FLyraAttributeEvent OnHungerChanged;
	mutable FLyraAttributeEvent OnMaxHungerChanged;
	mutable FLyraAttributeEvent OnThirstChanged;
	mutable FLyraAttributeEvent OnMaxThirstChanged;

protected:

	// Boilerplate OnRep functions required by Unreal Networking
	UFUNCTION() void OnRep_Strength(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Agility(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Intelligence(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Endurance(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Luck(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Willpower(const FGameplayAttributeData& OldValue);

	UFUNCTION() void OnRep_Defense(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_CriticalChance(const FGameplayAttributeData& OldValue);

	UFUNCTION() void OnRep_Hunger(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxHunger(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Thirst(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxThirst(const FGameplayAttributeData& OldValue);

	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:

	// Primary
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "OWRPG|Stats", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Strength;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Agility, Category = "OWRPG|Stats", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Agility;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intelligence, Category = "OWRPG|Stats", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Intelligence;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Endurance, Category = "OWRPG|Stats", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Endurance;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Luck, Category = "OWRPG|Stats", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Luck;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Willpower, Category = "OWRPG|Stats", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Willpower;

	// Combat
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Defense, Category = "OWRPG|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Defense;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalChance, Category = "OWRPG|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData CriticalChance;

	// Survival
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Hunger, Category = "OWRPG|Survival", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Hunger;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHunger, Category = "OWRPG|Survival", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHunger;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Thirst, Category = "OWRPG|Survival", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Thirst;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxThirst, Category = "OWRPG|Survival", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxThirst;
};