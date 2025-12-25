// Copyright Legion. All Rights Reserved.

#include "Equipment/OWRPGVisualItemActor.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"

AOWRPGVisualItemActor::AOWRPGVisualItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// CRITICAL: Ensure this actor exists on clients!
	bReplicates = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AOWRPGVisualItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Register the variable for replication
	DOREPLIFETIME(AOWRPGVisualItemActor, ItemMesh);
}

void AOWRPGVisualItemActor::SetItemMesh(UStaticMesh* NewMesh)
{
	// Only the Server can set this
	if (HasAuthority())
	{
		ItemMesh = NewMesh;
		OnRep_ItemMesh(); // Apply immediately on Server too
	}
}

void AOWRPGVisualItemActor::OnRep_ItemMesh()
{
	// This runs on Client (automatically) and Server (manually called above)
	if (MeshComp && ItemMesh)
	{
		MeshComp->SetStaticMesh(ItemMesh);
	}
}