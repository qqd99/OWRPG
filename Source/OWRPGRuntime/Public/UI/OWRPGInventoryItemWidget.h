// Copyright Legion. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "OWRPGInventoryItemWidget.generated.h"

class UImage;
class UTextBlock;
class UOWRPGInventoryManagerComponent;

UCLASS()
class OWRPGRUNTIME_API UOWRPGInventoryItemWidget : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	// Init now takes the visual flag
	void Init(ULyraInventoryItemInstance* InItem, UOWRPGInventoryManagerComponent* InManager, float InTileSize, bool bIsDragVisual = false);
	void Refresh(ULyraInventoryItemInstance* InItem);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	ULyraInventoryItemInstance* GetItemInstance() const { return ItemInstance.Get(); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UOWRPGInventoryManagerComponent* GetInventoryManager() const { return InventoryManager.Get(); }

	// NEW: Use this in Blueprint to stop logic if this is just a ghost
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsDragVisual() const { return bIsDragVisualWidget; }

	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	virtual void NativeDestruct() override;

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StackCountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY()
	TWeakObjectPtr<ULyraInventoryItemInstance> ItemInstance;

	UPROPERTY()
	TWeakObjectPtr<UOWRPGInventoryManagerComponent> InventoryManager;

	float TileSize = 50.0f;
	bool bIsDragVisualWidget = false;
};