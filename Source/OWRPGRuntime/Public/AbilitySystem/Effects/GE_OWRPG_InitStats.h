// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_OWRPG_InitStats.generated.h"

/**
 * UGE_OWRPG_InitStats
 * * C++ Gameplay Effect for initializing character stats.
 * Uses SetByCaller magnitudes so we can pass in Random values from code.
 */
UCLASS()
class OWRPGRUNTIME_API UGE_OWRPG_InitStats : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_OWRPG_InitStats();
};