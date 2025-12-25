// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Abilities/GameplayAbility.h"
#include "OWRPGWorldCollectable.generated.h"

class ULyraInventoryItemDefinition;
class UStaticMeshComponent; // Forward declaration

UCLASS()
class OWRPGRUNTIME_API AOWRPGWorldCollectable : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AOWRPGWorldCollectable();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OWRPG|Visuals")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "OWRPG|Item", meta = (ExposeOnSpawn = true))
	TSubclassOf<ULyraInventoryItemDefinition> StaticItemDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "OWRPG|Item", meta = (ExposeOnSpawn = true))
	int32 StackCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OWRPG|Interaction")
	TSubclassOf<UGameplayAbility> InteractionAbility;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& OptionBuilder) override;
};