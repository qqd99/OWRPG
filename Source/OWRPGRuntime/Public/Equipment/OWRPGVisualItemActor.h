// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OWRPGVisualItemActor.generated.h"

/**
 * A specialized actor for equipment visuals (items held in hand).
 * It automatically replicates the Mesh Asset to clients.
 */
UCLASS()
class OWRPGRUNTIME_API AOWRPGVisualItemActor : public AActor
{
	GENERATED_BODY()

public:
	AOWRPGVisualItemActor();

	// The Mesh Component (Visible in Editor)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	// --- REPLICATION ---
	// The variable that syncs across the network
	UPROPERTY(ReplicatedUsing = OnRep_ItemMesh)
	TObjectPtr<UStaticMesh> ItemMesh;

	// Function called on Client when ItemMesh updates
	UFUNCTION()
	void OnRep_ItemMesh();

	// Helper to set the mesh from the Server
	void SetItemMesh(UStaticMesh* NewMesh);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};