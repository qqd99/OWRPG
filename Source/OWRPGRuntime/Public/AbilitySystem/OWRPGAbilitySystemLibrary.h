// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OWRPGAbilitySystemLibrary.generated.h"

class UAbilitySystemComponent;

/**
 * Library for managing OWRPG Ability System logic (Initialization, etc).
 */
UCLASS()
class OWRPGRUNTIME_API UOWRPGAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
		 * Initializes stats with logic for Race and random variance.
		 * @param ASC           Target Ability System
		 * @param CharacterRace Tag defining the race (e.g., OWRPG.Race.RockPerson)
		 */
	UFUNCTION(BlueprintCallable, Category = "OWRPG|Initialization")
	static void InitializeRandomStats(UAbilitySystemComponent* ASC, FGameplayTag CharacterRace);
};