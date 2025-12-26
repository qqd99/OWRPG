// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OWRPGInventoryItemWidget.generated.h"

class ULyraInventoryItemInstance;
class UOWRPGInventoryManagerComponent;
class UImage;
class UTextBlock;

/**
 * Represents a single item in the inventory grid.
 */
UCLASS()
class OWRPGRUNTIME_API UOWRPGInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- DATA ---
	UPROPERTY(BlueprintReadWrite, Category = "Inventory", meta = (ExposeOnSpawn = true))
	TObjectPtr<ULyraInventoryItemInstance> ItemInstance;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory", meta = (ExposeOnSpawn = true))
	TObjectPtr<UOWRPGInventoryManagerComponent> InventoryManager;

	// Passed from Grid: Size of one cell in pixels
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (ExposeOnSpawn = true))
	float TileSize = 50.0f;

	// Passed from Grid: Is this item visually rotated?
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (ExposeOnSpawn = true))
	bool bIsRotated = false;

	// --- CONFIG ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> TooltipWidgetClass;

	// The Widget Class to spawn when Right-Clicked (Context Menu)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> ContextMenuClass;

protected:
	// --- UI BINDINGS (Must match Widget Blueprint names) ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StackText;

public:
	// Forces visual refresh (Size, Icon, Rotation, Stack)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	void UpdateVisual();

protected:
	// --- EVENTS ---

	// Implement this in BP to initialize the Context Menu (e.g. Set Item reference)
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnContextOpened(UUserWidget* ContextMenuWidget, ULyraInventoryItemInstance* Item);

	// --- OVERRIDES ---
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	// Internal helper to spawn the context menu
	void CreateContextMenu();
};