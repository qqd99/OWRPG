// Copyright Legion. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraAttributeSet.h"
#include "NativeGameplayTags.h"

#include "OWRPGManaSet.generated.h"

/**
 * UOWRPGManaSet
 *
 * Class that defines attributes that are necessary for the mana system.
 * Attributes: Mana, MaxMana, ManaRegenRate
 */
UCLASS(BlueprintType)
class OWRPGRUNTIME_API UOWRPGManaSet : public ULyraAttributeSet
{
	GENERATED_BODY()

public:

	UOWRPGManaSet();

	ATTRIBUTE_ACCESSORS(UOWRPGManaSet, Mana);
	ATTRIBUTE_ACCESSORS(UOWRPGManaSet, MaxMana);
	ATTRIBUTE_ACCESSORS(UOWRPGManaSet, ManaRegenRate);

	// Delegate to broadcast when the attribute changes.
	mutable FLyraAttributeEvent OnManaChanged;
	mutable FLyraAttributeEvent OnMaxManaChanged;
	mutable FLyraAttributeEvent OnManaRegenRateChanged;

protected:

	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ManaRegenRate(const FGameplayAttributeData& OldValue);

	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:

	// The current mana attribute.  The mana will be capped by the MaxMana attribute.  The mana is hidden from modifiers so only the ManaRegenRate will be able to modify the mana.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "OWRPG|Mana", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Mana;

	// The current max mana attribute.  MaxMana is an attribute since GameplayEffects can modify it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "OWRPG|Mana", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxMana;

	// The current mana regen rate attribute.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ManaRegenRate, Category = "OWRPG|Mana", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ManaRegenRate;

	// Track whether we have applied the "Draining" tag to the ASC
	bool bOutOfMana;
};