// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OWRPGInventoryItemWidget.generated.h"

class ULyraInventoryItemInstance;
class UOWRPGInventoryManagerComponent;
class UImage;

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

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	float TileSize = 50.0f;

	// --- UI BINDINGS ---
	// You MUST name your Image widget "IconImage" in the Widget Designer!
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> TooltipWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> ContextMenuClass;

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	void UpdateVisual();

protected:
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	void CreateContextMenu();
};