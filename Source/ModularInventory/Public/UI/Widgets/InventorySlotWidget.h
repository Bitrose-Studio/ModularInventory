// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryComponent.h"
#include "InventorySlotWidget.generated.h"

class UInventoryDragDropOperation;
/**
 * 
 */
UCLASS(Blueprintable)
class MODULARINVENTORY_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** The item this slot is currently representing. */
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|UI")
	FInventoryEntry ItemData;
	
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|UI")
	bool bIsEmpty = true;

	/** Index of this slot in the container (optional, for grids/hotbars). */
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|UI")
	int32 SlotIndex = INDEX_NONE;

	/** The inventory component this slot belongs to. */
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|UI")
	TObjectPtr<UInventoryComponent> OwningInventory;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Modular Inventory|UI")
	TSubclassOf<UUserWidget> DragVisualClass;

	/** Initialize slot from an inventory item. Call right after creating the widget. */
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|UI")
	void SetupSlot(UInventoryComponent* InInventory, int32 InSlotIndex, const FInventoryEntry& InItem);
	
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|UI")
	void SetupEmpty(UInventoryComponent* InInventory, int32 InSlotIndex);

protected:
	
	/** Called after SetupSlot; implement in Blueprint to update visuals (icon, text, etc.). */
	UFUNCTION(BlueprintImplementableEvent, Category="Modular Inventory|UI")
	void OnItemDataSet();
	
	UFUNCTION(BlueprintImplementableEvent, Category="Modular Inventory|UI")
	void OnEmptySlot();
	
	// Drag & drop overrides
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	
	/** True if the current drag was started with the right mouse button (split drag). */
	bool bIsRightMouseDrag = false;
};
