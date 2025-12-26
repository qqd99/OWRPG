// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OWRPGInventoryGridWidget.generated.h"

class UOWRPGInventoryManagerComponent;
class UOWRPGInventoryItemWidget;
class UCanvasPanel;
class UBorder;

/**
 * The Visual Grid.
 * Handles Drag Over, Drop, and Rotation Input ('R' key).
 */
UCLASS()
class OWRPGRUNTIME_API UOWRPGInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- DEPENDENCIES ---
	UPROPERTY(BlueprintReadWrite, Category = "Inventory", meta = (ExposeOnSpawn = true))
	TObjectPtr<UOWRPGInventoryManagerComponent> InventoryManager;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UOWRPGInventoryItemWidget> ItemWidgetClass;

	// --- CONFIG ---
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float TileSize = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FLinearColor ValidDropColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.3f);

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FLinearColor InvalidDropColor = FLinearColor(1.0f, 0.0f, 0.0f, 0.3f);

protected:
	// --- WIDGET BINDINGS ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> GridCanvas;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BackgroundBorder;

	// --- INTERNAL STATE ---
	UPROPERTY()
	TMap<TObjectPtr<class ULyraInventoryItemInstance>, TObjectPtr<UOWRPGInventoryItemWidget>> ItemWidgets;

	// Drag State
	bool bIsDraggingOver = false;
	bool bIsPlacementValid = false;

	// Rotation State (New)
	bool bIsDraggingRotated = false;

	int32 HoveredX = -1;
	int32 HoveredY = -1;
	int32 DraggedW = 1;
	int32 DraggedH = 1;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeGrid(UOWRPGInventoryManagerComponent* InManager);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshGrid();

protected:
	virtual void NativeDestruct() override;

	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	// Helper to flip dimensions if R is pressed
	void CheckRotationInput();
};