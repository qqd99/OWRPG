// Copyright Legion. All Rights Reserved.

#include "Interaction/OWRPGWorldCollectable.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/OWRPGInventoryFragment_UI.h"
#include "Inventory/OWRPGInventoryFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "DrawDebugHelpers.h" // For Viewport warnings

AOWRPGWorldCollectable::AOWRPGWorldCollectable()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);
	bNetLoadOnClient = true;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = StaticMeshComponent;

	// Physics Setup
	StaticMeshComponent->SetSimulatePhysics(true);
	StaticMeshComponent->SetCollisionProfileName(TEXT("OWRPG_Pickup"));

	// FIX: EXPLOSIVE SEPARATION
	// Default is often high (e.g., 1000+). Lowering this makes them gently push apart.
	StaticMeshComponent->BodyInstance.SetMaxDepenetrationVelocity(100.0f);

	// Optional: Add Damping (Air Resistance) so they don't roll forever
	StaticMeshComponent->SetLinearDamping(1.0f);  // Makes it feel heavier
	StaticMeshComponent->SetAngularDamping(1.0f); // Stops it spinning like a top
}

void AOWRPGWorldCollectable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOWRPGWorldCollectable, StaticItemDefinition);
	DOREPLIFETIME(AOWRPGWorldCollectable, StackCount);
}

void AOWRPGWorldCollectable::BeginPlay()
{
	Super::BeginPlay();

	// SERVER-SIDE VALIDATION: If this item has no definition, it's useless.
	if (HasAuthority() && !StaticItemDefinition)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] SPATIAL ITEM HAS NO DEFINITION! Destroying to prevent errors."), *GetName());
		Destroy();
	}
}

void AOWRPGWorldCollectable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	// Validation Warning
	if (!StaticItemDefinition)
	{
		DrawDebugString(GetWorld(), FVector(0, 0, 50), TEXT("MISSING ITEM DEF!"), this, FColor::Red, 0.0f);
		return;
	}

	// Get the Definition
	const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(StaticItemDefinition);
	if (!Def) return;

	// VISUAL AUTOMATION (Mesh)
	if (const UOWRPGInventoryFragment_UI* UIFrag = UOWRPGInventoryFunctionLibrary::FindItemDefinitionFragment<UOWRPGInventoryFragment_UI>(Def))
	{
		if (UIFrag->WorldMesh && StaticMeshComponent)
		{
			StaticMeshComponent->SetStaticMesh(UIFrag->WorldMesh);
		}
	}

	// AUTO-RENAME ACTOR (Outliner)
	// Get the User-Friendly Name (e.g., "Iron Sword")
	FString NewLabel = Def->DisplayName.ToString();

	// Fallback if Display Name is empty
	if (NewLabel.IsEmpty())
	{
		NewLabel = Def->GetName();
		// Clean up the name (remove "ID_" prefix if you want)
		NewLabel.RemoveFromStart("ID_");
	}

	// Set the Actor Label in the World Outliner
	// We add "Pickup_" prefix to keep them sorted together
	SetActorLabel(FString::Printf(TEXT("Pickup_%s"), *NewLabel));

#endif
}

void AOWRPGWorldCollectable::GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& OptionBuilder)
{
	// If we are broken or pending kill, don't interact.
	if (!StaticItemDefinition || !InteractionAbility)
	{
		return;
	}

	FInteractionOption Option;

	Option.InteractionAbilityToGrant = InteractionAbility;
	Option.InteractableTarget = this;

	// --- GENERATE UI TEXT ---
	FText ItemName = FText::FromString("Unknown Item");

	if (const ULyraInventoryItemDefinition* Def = GetDefault<ULyraInventoryItemDefinition>(StaticItemDefinition))
	{
		if (!Def->DisplayName.IsEmpty())
		{
			ItemName = Def->DisplayName;
		}
		else
		{
			ItemName = FText::FromString(Def->GetName());
		}
	}

	Option.Text = FText::Format(FText::FromString("Pick Up {0}"), ItemName);
	Option.SubText = FText::FromString(FString::Printf(TEXT("x%d"), StackCount));

	OptionBuilder.AddInteractionOption(Option);
}