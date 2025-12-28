// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "OWRPGInventoryGridWidget.generated.h"

class UOWRPGInventoryManagerComponent;
class UCanvasPanel;
class USizeBox;
class UOWRPGInventoryItemWidget;
class ULyraInventoryItemInstance;
class UBorder;

/**
 * Spatial Grid with Dynamic Resizing and Widget Pooling.
 * O(N) refresh complexity instead of O(Widgets).
 */
UCLASS()
class OWRPGRUNTIME_API UOWRPGInventoryGridWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// --- SETUP ---

	/** * Binds this UI to a specific Manager (e.g., Player Inventory or Chest).
	 * Automatically resizes the grid to fit the Manager's Column/Row count.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeGrid(UOWRPGInventoryManagerComponent* InManager);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshGrid();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	// --- CONFIG ---
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float TileSize = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UOWRPGInventoryItemWidget> ItemWidgetClass;

	// --- COMPONENTS ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> GridSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> GridCanvas;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> BackgroundCanvas;

	// The "Ghost" highlight. Add a Border named "DragHighlight" to W_InventoryGrid!
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DragHighlight;

	// --- POOLING SYSTEM ---
	UPROPERTY()
	TArray<TObjectPtr<UOWRPGInventoryItemWidget>> WidgetPool;

	UPROPERTY()
	TMap<TObjectPtr<ULyraInventoryItemInstance>, TObjectPtr<UOWRPGInventoryItemWidget>> ActiveItemWidgets;

	UPROPERTY()
	TObjectPtr<UOWRPGInventoryManagerComponent> InventoryManager;

	// --- HELPERS ---
	UOWRPGInventoryItemWidget* GetFreeWidget();
	void DrawGridLines();

	// Drag Preview
	UPROPERTY()
	TObjectPtr<UUserWidget> DragHighlightWidget;
};