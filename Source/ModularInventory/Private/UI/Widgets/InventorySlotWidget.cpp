// Copyright Peter Gyarmati (BitroseStudio)


#include "UI/Widgets/InventorySlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Inventory/InventoryItemInstance.h"
#include "Inventory/Fragments/InventoryItemFragment.h"
#include "Inventory/Fragments/ItemFragment_UserInterface.h"
#include "UI/InventoryDragDropOperation.h"
#include "UI/Widgets/InventoryDragVisualWidget.h"

void UInventorySlotWidget::SetupSlot(UInventoryComponent* InInventory, int32 InSlotIndex, const FInventoryEntry& InItem)
{
	OwningInventory = InInventory;
	SlotIndex = InSlotIndex;
	ItemData = InItem;
	bIsEmpty = false;
	
	OnItemDataSet(); // BP: update icon, name, quantity, etc.
}

void UInventorySlotWidget::SetupEmpty(UInventoryComponent* InInventory, int32 InSlotIndex)
{
	OwningInventory = InInventory;
	SlotIndex       = InSlotIndex;
	ItemData        = FInventoryEntry(); // reset
	bIsEmpty        = true;

	OnEmptySlot();
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsEmpty)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FKey Button = InMouseEvent.GetEffectingButton();

	// Left mouse: drag full stack
	if (Button == EKeys::LeftMouseButton)
	{
		bIsRightMouseDrag = false;

		const FEventReply EventReply = UWidgetBlueprintLibrary::DetectDragIfPressed(
			InMouseEvent, this, EKeys::LeftMouseButton);

		return EventReply.NativeReply;
	}
	// Right mouse: drag half stack (if more than 1)
	else if (Button == EKeys::RightMouseButton && ItemData.GetQuantity() > 1)
	{
		bIsRightMouseDrag = true;

		const FEventReply EventReply = UWidgetBlueprintLibrary::DetectDragIfPressed(
			InMouseEvent, this, EKeys::RightMouseButton);

		return EventReply.NativeReply;
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	
	if (!OwningInventory || bIsEmpty)
	{
		bIsRightMouseDrag = false;
		return;
	}

	UInventoryDragDropOperation* DragOp = NewObject<UInventoryDragDropOperation>();
	DragOp->SourceInventory = OwningInventory;
	DragOp->SourceSlotIndex = SlotIndex;
	DragOp->ItemGuid        = ItemData.GetItemGuid();

	if (bIsRightMouseDrag && ItemData.GetQuantity() > 1)
	{
		const int32 HalfQuantity = ItemData.GetQuantity() / 2;
		DragOp->bIsSplitDrag  = true;
		DragOp->SplitQuantity = HalfQuantity;
		DragOp->Quantity      = HalfQuantity;
	}
	else
	{
		DragOp->bIsSplitDrag  = false;
		DragOp->SplitQuantity = 0;
		DragOp->Quantity      = ItemData.GetQuantity();
	}
	
	// 🔹 Get icon from the item’s UI fragment
	UTexture2D* IconTex = nullptr;
	if (ItemData.IsItemInstanceValid())
	{
		const UInventoryItemFragment* FragBase =
		ItemData.GetItemInstance()->FindFragmentByClass(UItemFragment_UserInterface::StaticClass());
		if (const UItemFragment_UserInterface* UIFrag = Cast<UItemFragment_UserInterface>(FragBase))
		{
			IconTex = UIFrag->GetIcon();
			// DisplayName, Description, etc.
		}
	}
	DragOp->Icon = IconTex;

	if (DragVisualClass)
	{
		UInventoryDragVisualWidget* Visual = CreateWidget<UInventoryDragVisualWidget>(GetWorld(), DragVisualClass);
		// Optionally, implement a BP interface or exposed function on this widget
		// e.g. IInventoryDragVisual::InitFromDragOp(DragOp) to set icon + quantity.
		Visual->DragIcon = DragOp->Icon;
		Visual->DragQuantity = DragOp->Quantity;
		DragOp->DefaultDragVisual = Visual;
	}
	else
	{
		DragOp->DefaultDragVisual = this;
	}
	
	bIsRightMouseDrag = false;
	OutOperation      = DragOp;
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (!InOperation)
	{
		return false;
	}

	UInventoryDragDropOperation* DragOp = Cast<UInventoryDragDropOperation>(InOperation);
	if (!DragOp || !OwningInventory)
	{
		return false;
	}

	// ---- Split drag (right mouse) ----
	if (DragOp->bIsSplitDrag && DragOp->SplitQuantity > 0)
	{
		if (!DragOp->SourceInventory || SlotIndex == INDEX_NONE || !DragOp->ItemGuid.IsValid())
		{
			return false;
		}

		// Same or different inventory both go through the same RPC:
		// "split in SourceInventory, move half to OwningInventory at SlotIndex"
		DragOp->SourceInventory->SplitAndMoveItem(
			DragOp->ItemGuid,
			DragOp->SplitQuantity,
			OwningInventory,
			SlotIndex
		);

		return true;
	}

	// ---- Normal drag (left mouse, full stack) ----

	// Same inventory → reorder/move by Guid
	if (DragOp->SourceInventory == OwningInventory)
	{
		if (SlotIndex != INDEX_NONE && DragOp->ItemGuid.IsValid())
		{
			OwningInventory->MoveItemByGuid(DragOp->ItemGuid, SlotIndex);
			return true;
		}
		return false;
	}

	// Different inventories → move full stack
	if (DragOp->ItemGuid.IsValid() && DragOp->SourceInventory)
	{
		DragOp->SourceInventory->MoveItemToInventory(
			OwningInventory,
			DragOp->ItemGuid
		);
		return true;
	}

	return false;
}
