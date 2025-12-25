// Copyright Legion. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraAttributeSet.h"
#include "NativeGameplayTags.h"

#include "OWRPGStaminaSet.generated.h"

/**
 * UOWRPGStaminaSet
 *
 * Class that defines attributes that are necessary for the stamina system.
 * Attributes: Stamina, MaxStamina, StaminaRegenRate
 */
UCLASS(BlueprintType)
class OWRPGRUNTIME_API UOWRPGStaminaSet : public ULyraAttributeSet
{
	GENERATED_BODY()

public:

	UOWRPGStaminaSet();

	ATTRIBUTE_ACCESSORS(UOWRPGStaminaSet, Stamina);
	ATTRIBUTE_ACCESSORS(UOWRPGStaminaSet, MaxStamina);
	ATTRIBUTE_ACCESSORS(UOWRPGStaminaSet, StaminaRegenRate);

	// Delegate to broadcast when the attribute changes.
	mutable FLyraAttributeEvent OnStaminaChanged;
	mutable FLyraAttributeEvent OnMaxStaminaChanged;
	mutable FLyraAttributeEvent OnStaminaRegenRateChanged;

protected:

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue);

	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:

	// The current stamina attribute.  The stamina will be capped by the MaxStamina attribute.  The stamina is hidden from modifiers so only the StaminaRegenRate will be able to modify the stamina.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "OWRPG|Stamina", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Stamina;

	// The current max stamina attribute.  MaxStamina is an attribute since GameplayEffects can modify it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "OWRPG|Stamina", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxStamina;

	// The current stamina regen rate attribute.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegenRate, Category = "OWRPG|Stamina", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData StaminaRegenRate;

	// Track whether we have applied the "Draining" tag to the ASC
	bool bOutOfStamina;
};