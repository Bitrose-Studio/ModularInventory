// Copyright Peter Gyarmati (BitroseStudio)


#include "UI/Widgets/InventoryPanelWidget.h"

#include "Components/UniformGridPanel.h"
#include "UI/Widgets/InventorySlotWidget.h"

void UInventoryPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (SourceInventory)
		InitializeWithInventory(SourceInventory);
}

void UInventoryPanelWidget::NativeDestruct()
{
	if (SourceInventory)
	{
		SourceInventory->OnItemAdded.RemoveDynamic(this, &UInventoryPanelWidget::HandleItemAdded);
		SourceInventory->OnItemRemoved.RemoveDynamic(this, &UInventoryPanelWidget::HandleItemRemoved);
		SourceInventory->OnItemChanged.RemoveDynamic(this, &UInventoryPanelWidget::HandleItemChanged);
		SourceInventory->OnInventoryRefreshed.RemoveDynamic(this, &UInventoryPanelWidget::HandleInventoryRefreshed);
		SourceInventory->OnMaxSlotsChanged.RemoveDynamic(this, &UInventoryPanelWidget::HandleMaxSlotsChanged);
	}
	
	Super::NativeDestruct();
}

void UInventoryPanelWidget::InitializeWithInventory(UInventoryComponent* InInventory)
{
	if (!InInventory) return;

	// Unbind from previous
	if (SourceInventory)
	{
		SourceInventory->OnItemAdded.RemoveDynamic(this, &UInventoryPanelWidget::HandleItemAdded);
		SourceInventory->OnItemRemoved.RemoveDynamic(this, &UInventoryPanelWidget::HandleItemRemoved);
		SourceInventory->OnItemChanged.RemoveDynamic(this, &UInventoryPanelWidget::HandleItemChanged);
		SourceInventory->OnInventoryRefreshed.RemoveDynamic(this, &UInventoryPanelWidget::HandleInventoryRefreshed);
		SourceInventory->OnMaxSlotsChanged.RemoveDynamic(this, &UInventoryPanelWidget::HandleMaxSlotsChanged);
	}

	SourceInventory = InInventory;

	// Bind new
	SourceInventory->OnItemAdded.AddDynamic(this, &UInventoryPanelWidget::HandleItemAdded);
	SourceInventory->OnItemRemoved.AddDynamic(this, &UInventoryPanelWidget::HandleItemRemoved);
	SourceInventory->OnItemChanged.AddDynamic(this, &UInventoryPanelWidget::HandleItemChanged);
	SourceInventory->OnInventoryRefreshed.AddDynamic(this, &UInventoryPanelWidget::HandleInventoryRefreshed);
	SourceInventory->OnMaxSlotsChanged.AddDynamic(this, &UInventoryPanelWidget::HandleMaxSlotsChanged);

	RebuildFromInventory();
}

void UInventoryPanelWidget::RebuildFromInventory()
{
	if (!SourceInventory || !ItemsPanel || !SlotWidgetClass)
	{
		return;
	}

	ItemsPanel->ClearChildren();

	const TArray<FInventoryEntry>& Items = SourceInventory->GetInventoryEntries().GetAllEntriesRef();
	const int32 MaxSlots = SourceInventory->GetMaxSlots();
	const int32 NumSlotsToShow = (MaxSlots > 0) ? MaxSlots : Items.Num();
	
	for (const auto Item : Items)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %d"), *Item.GetDebugString(), Item.GetQuantity());
	}

	for (int32 SlotIndex = 0; SlotIndex < NumSlotsToShow; ++SlotIndex)
	{
		UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		const int32 Row = SlotIndex / NumColumns;
		const int32 Col = SlotIndex % NumColumns;

		// Find item that belongs to this logical slot
		const FInventoryEntry* FoundItem = Items.FindByPredicate(
			[SlotIndex](const FInventoryEntry& Item)
			{
				return Item.GetSlotIndex() == SlotIndex;
			});
		
		if (FoundItem)
		{
			SlotWidget->SetupSlot(SourceInventory, SlotIndex, *FoundItem);
		}
		else
		{
			SlotWidget->SetupEmpty(SourceInventory, SlotIndex);
		}

		ItemsPanel->AddChildToUniformGrid(SlotWidget, Row, Col);
	}
	
	OnPanelRebuilt();
}

void UInventoryPanelWidget::HandleItemAdded(const FInventoryEntry& Item)
{
	RebuildFromInventory();
}

void UInventoryPanelWidget::HandleItemRemoved(const FInventoryEntry& Item)
{
	RebuildFromInventory();
}

void UInventoryPanelWidget::HandleItemChanged(const FInventoryEntry& Item)
{
	RebuildFromInventory();
}

void UInventoryPanelWidget::HandleInventoryRefreshed(const TArray<FInventoryEntry>& Items)
{
	RebuildFromInventory();
}

void UInventoryPanelWidget::HandleMaxSlotsChanged(int32 NewMaxSlots)
{
}
