// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragDropOperation.generated.h"

class UInventoryComponent;
/**
 * 
 */
UCLASS()
class MODULARINVENTORY_API UInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()
	
public:
	/** The inventory we dragged from. */
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|DragDrop")
	TObjectPtr<UInventoryComponent> SourceInventory = nullptr;

	/** Slot index in the source inventory. */
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|DragDrop")
	int32 SourceSlotIndex = INDEX_NONE;

	/** Guid of the item stack being dragged. */
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|DragDrop")
	FGuid ItemGuid;

	/** Quantity being dragged (for now: whole stack). */
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|DragDrop")
	int32 Quantity = 0;
	
	/** Optional icon for drag visual. */
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|DragDrop")
	TObjectPtr<UTexture2D> Icon = nullptr;
	
	/** True if this drag is a split (right-mouse). */
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|DragDrop")
	bool bIsSplitDrag = false;

	/** How much we intend to split off. */
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|DragDrop")
	int32 SplitQuantity = 0;
};
