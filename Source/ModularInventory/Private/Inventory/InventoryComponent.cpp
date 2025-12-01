// Copyright Peter Gyarmati (BitroseStudio)


#include "Inventory/InventoryComponent.h"

#include "DataAssets/InventoryItemDefinition.h"
#include "DataAssets/InventoryLootTable.h"
#include "Engine/ActorChannel.h"
#include "Inventory/InventoryItemInstance.h"
#include "Inventory/Fragments/ItemFragment_Stackable.h"
#include "Net/UnrealNetwork.h"


void FInventoryEntry::PreReplicatedRemove(const struct FInventoryList& Serializer)
{
	UE_LOG(LogTemp, Log, TEXT("[FInventoryEntry] PreReplicatedRemove"));
}

void FInventoryEntry::PostReplicatedAdd(const struct FInventoryList& Serializer)
{
	UE_LOG(LogTemp, Log, TEXT("[FInventoryEntry] PostReplicatedAdd"));
}

void FInventoryEntry::PostReplicatedChange(const struct FInventoryList& Serializer)
{
	UE_LOG(LogTemp, Log, TEXT("[FInventoryEntry] PostReplicatedChange"));
}

UInventoryItemInstance* FInventoryEntry::GetItemInstance() const
{
	if (!ItemInstance) return nullptr;
	return ItemInstance;
}

TArray<UInventoryItemInstance*> FInventoryList::GetAllItems() const
{
	TArray<UInventoryItemInstance*> Results;
	Results.Reserve(Entries.Num());
	for (const auto& Entry : Entries)
	{
		if (!Entry.ItemInstance) continue;
		Results.Add(Entry.ItemInstance);
	}
	return Results;
}

void FInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	UInventoryComponent* InventoryComponent = Cast<UInventoryComponent>(OwnerComponent);
	if (!IsValid(InventoryComponent)) return;
	
	for (int32 Index : RemovedIndices)
	{
		InventoryComponent->OnItemRemoved.Broadcast(Entries[Index]);
		UE_LOG(LogTemp, Log, TEXT("[FInventoryList] PreReplicationRemove: %d"), Index);
	}
		
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UInventoryComponent* InventoryComponent = Cast<UInventoryComponent>(OwnerComponent);
	if (!IsValid(InventoryComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("[FInventoryList] PostReplicatedAdd: No OwnerComponent!")); 
		return;
	}
	
	for (int32 Index : AddedIndices)
	{
		InventoryComponent->OnItemAdded.Broadcast(Entries[Index]);
		UE_LOG(LogTemp, Log, TEXT("[FInventoryList] PostReplicatedAdd: %d"), Index);
	}
	UE_LOG(LogTemp, Warning, TEXT("[FInventoryList] PostReplicatedAdd: End of the function."));	
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	UInventoryComponent* InventoryComponent = Cast<UInventoryComponent>(OwnerComponent);
	if (!IsValid(InventoryComponent)) return;
	
	for (int32 Index : ChangedIndices)
	{
		InventoryComponent->OnItemChanged.Broadcast(Entries[Index]);
		UE_LOG(LogTemp, Log, TEXT("[FInventoryList] PostReplicatedChange: %d"), Index);
	}
		
}

void FInventoryList::AddItem(UInventoryItemInstance* Instance, int32 Quantity)
{
	checkf(OwnerComponent, TEXT("Error! OwnerComponent not set."));
	
	if (UInventoryComponent* InventoryComponent = Cast<UInventoryComponent>(OwnerComponent))
	{
		AddItemToSlot(Instance, Quantity, InventoryComponent->FindFirstFreeSlotIndex(), InventoryComponent);
		return;
	}
	UE_LOG(LogTemp, Error, TEXT("Owner Component Cast Failed!"));
}

void FInventoryList::AddItem(UInventoryItemInstance* Instance, int32 Quantity, int32 PreferredSlotIndex)
{
	checkf(OwnerComponent, TEXT("Error! OwnerComponent not set."));
	
	if (UInventoryComponent* InventoryComponent = Cast<UInventoryComponent>(OwnerComponent))
	{
		AddItemToSlot(Instance, Quantity, PreferredSlotIndex, InventoryComponent);
		return;
	}
	UE_LOG(LogTemp, Error, TEXT("Owner Component Cast Failed!"));
}

void FInventoryList::AddItemToSlot(UInventoryItemInstance* Instance, int32 Quantity, int32 SlotIndex, UInventoryComponent* InOwnerComponent)
{	
	FInventoryEntry NewEntry;
	NewEntry.ItemInstance = Instance;
	NewEntry.Quantity = Quantity;
	NewEntry.ItemGuid = FGuid::NewGuid();
	NewEntry.SlotIndex = SlotIndex;
	
	UE_LOG(LogTemp, Log, TEXT("[InventoryList] AddItem: %s"),
		*NewEntry.GetDebugString());
	
	Entries.Add(NewEntry);
	MarkItemDirty(NewEntry);
	
	InOwnerComponent->PostInventoryItemAdded(NewEntry);
}

bool FInventoryList::RemoveItem(const FGuid& ItemGuid, int32 QuantityToRemove)
{
	int32 Index = Entries.IndexOfByPredicate([&](const FInventoryEntry& Entry)
	{
		return Entry.ItemGuid == ItemGuid;
	});

	if (Index == INDEX_NONE)
		return false;

	FInventoryEntry& Entry = Entries[Index];

	if (QuantityToRemove <= 0 || QuantityToRemove > Entry.Quantity)
		return false;

	Entry.Quantity -= QuantityToRemove;
	MarkItemDirty(Entry);

	if (Entry.Quantity <= 0)
	{
		// Make a copy for the callback before we remove the element
		FInventoryEntry RemovedEntry = Entry;
		
		Entries.RemoveAt(Index);
		MarkArrayDirty();
		
		if (OwnerComponent)
		{
			Cast<UInventoryComponent>(OwnerComponent)->PostInventoryItemRemoved(RemovedEntry);
		}
	}
	else
	{
		if (OwnerComponent)
		{
			Cast<UInventoryComponent>(OwnerComponent)->PostInventoryItemChanged(Entry);
		}
	}

	return true;
}

UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	InventoryEntries.SetOwnerComponent(this);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	if (ContainerType == EInventoryContainerType::PlayerInventory || ContainerType == EInventoryContainerType::Hotbar)
	{
		DOREPLIFETIME_CONDITION(ThisClass, InventoryEntries, COND_OwnerOnly);
		DOREPLIFETIME_CONDITION(ThisClass, MaxSlots, COND_OwnerOnly);
	}
	
	DOREPLIFETIME(ThisClass, InventoryEntries);
	DOREPLIFETIME(ThisClass, MaxSlots);
}

int32 UInventoryComponent::FindFirstFreeSlotIndex() const
{
	const TArray<FInventoryEntry>& CachedEntries = InventoryEntries.GetAllEntries();
	
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		const bool bIsTaken = CachedEntries.ContainsByPredicate([i](const FInventoryEntry& Entry) { return Entry.SlotIndex == i; });
		if (!bIsTaken) return i;
	}
	return CachedEntries.Num();
}

void UInventoryComponent::SetMaxSlots(int32 NewMaxSlots)
{
	NewMaxSlots = FMath::Max(NewMaxSlots, 0);
	if (MaxSlots == NewMaxSlots)
	{
		return;
	}

	MaxSlots = NewMaxSlots;
	OnRep_MaxSlots();
}

int32 UInventoryComponent::GetFreeSlotCount() const
{
	const int32 UsedSlots = InventoryEntries.GetEntriesCount();
	return FMath::Max(0, MaxSlots - UsedSlots);
}

bool UInventoryComponent::FindItemByGuid(const FGuid& ItemGuid, FInventoryEntry& OutItem) const
{
	const FInventoryEntry* Found = InventoryEntries.GetAllEntries().FindByPredicate(
		[&ItemGuid](const FInventoryEntry& Item)
		{
			return Item.ItemGuid == ItemGuid;
		});

	if (Found)
	{
		OutItem = *Found;
		return true;
	}
	return false;
}

void UInventoryComponent::TryAddItem(const UInventoryItemDefinition* ItemDef)
{
	unimplemented();
}

bool UInventoryComponent::AddItem(const UInventoryItemDefinition* ItemDef, int32 Quantity)
{
	if (!ItemDef || Quantity <= 0) return false;
	
	if (!CanAcceptItemDefinition(ItemDef))
	{
		UE_LOG(LogTemp, Log, TEXT("[InventoryComponent] AddItem: %s rejected by tag filter"),
			*GetNameSafe(ItemDef));
		return false;
	}

	const int32 RequestedQuantity = Quantity;

	// Stackable trait from fragment on THIS asset
	const UItemFragment_Stackable* StackableFragment =
		Cast<UItemFragment_Stackable>(
			ItemDef->FindFragmentByClass(UItemFragment_Stackable::StaticClass())
		);

	UE_LOG(LogTemp, Log, TEXT("[InventoryComponent] AddItem: %s x%d (Stackable=%s, MaxSlots=%d, Used=%d)"),
		*GetNameSafe(ItemDef),
		Quantity,
		StackableFragment ? TEXT("true") : TEXT("false"),
		MaxSlots,
		InventoryEntries.GetEntriesCount());

	// 1) Fill existing stacks
	if (StackableFragment)
	{
		const int32 MaxStackSize = StackableFragment->GetMaxStackLimit();

		for (FInventoryEntry& Entry : InventoryEntries.GetAllEntriesRef())
		{
			if (Entry.ItemInstance && Entry.ItemInstance->ItemDef == ItemDef)
			{
				const int32 RemainingSpace = MaxStackSize - Entry.Quantity;
				if (RemainingSpace > 0)
				{
					const int32 ToAdd = FMath::Min(RemainingSpace, Quantity);
					Entry.Quantity += ToAdd;
					Quantity       -= ToAdd;

					InventoryEntries.MarkItemDirty(Entry);

					if (GetOwnerRole() == ROLE_Authority)
					{
						PostInventoryItemChanged(Entry);
					}

					if (Quantity <= 0)
					{
						return true; // fully added
					}
				}
			}
		}
	}

	// 2) Create new stacks
	while (Quantity > 0)
	{
		// Capacity check: 0 = unlimited slots
		if (MaxSlots > 0 && InventoryEntries.GetEntriesCount() >= MaxSlots)
		{
			UE_LOG(LogTemp, Log, TEXT("[InventoryComponent] AddItem: no free slots left, stopping (Remaining=%d)"),
				Quantity);
			break;
		}

		int32 AddQuantity = 1;
		if (StackableFragment)
		{
			const int32 MaxStackSize = StackableFragment->GetMaxStackLimit();
			AddQuantity = FMath::Min(Quantity, MaxStackSize);
		}

		// Use the asset as definition for the instance
		UInventoryItemInstance* NewInstance = CreateItemInstance(ItemDef);
		if (!NewInstance)
		{
			UE_LOG(LogTemp, Error, TEXT("[InventoryComponent] AddItem: failed to create instance for %s"),
				*GetNameSafe(ItemDef));
			break;
		}

		InventoryEntries.AddItem(NewInstance, AddQuantity);
		Quantity -= AddQuantity;
	}

	const bool bFullyAdded = (Quantity <= 0);
	if (!bFullyAdded)
	{
		UE_LOG(LogTemp, Log, TEXT("[InventoryComponent] AddItem: partially added %d of %d"),
			RequestedQuantity - Quantity, RequestedQuantity);
	}
	return bFullyAdded;
}

bool UInventoryComponent::RemoveItem(const FGuid& ItemGuid, int32 QuantityToRemove)
{
	return InventoryEntries.RemoveItem(ItemGuid, QuantityToRemove);
}

bool UInventoryComponent::SwapItems(int32 SlotIndexA, int32 SlotIndexB)
{
	if (SlotIndexA == SlotIndexB)
	{
		return false;
	}

	if (!InventoryEntries.GetAllEntries().IsValidIndex(SlotIndexA) ||
		!InventoryEntries.GetAllEntries().IsValidIndex(SlotIndexB))
	{
		return false;
	}

	FInventoryEntry& A = InventoryEntries.GetAllEntries()[SlotIndexA];
	FInventoryEntry& B = InventoryEntries.GetAllEntries()[SlotIndexB];

	InventoryEntries.GetAllEntries().Swap(SlotIndexA, SlotIndexB);

	// Mark both dirty so replication + UI picks up change
	InventoryEntries.MarkItemDirty(A);
	InventoryEntries.MarkItemDirty(B);

	// Optional: force a full refresh event
	OnInventoryRefreshed.Broadcast(InventoryEntries.GetAllEntries());

	return true;
}

bool UInventoryComponent::SplitItemStack(const FGuid& ItemGuid, int32 SplitQuantity)
{
	FGuid DummyGUID;
	return SplitItemStackForDrag(ItemGuid, SplitQuantity, DummyGUID);
}

bool UInventoryComponent::SplitItemStackForDrag(const FGuid& ItemGuid, int32 SplitQuantity, FGuid& OutNewStackGuid)
{
	OutNewStackGuid.Invalidate();

	int32 Index = InventoryEntries.GetAllEntries().IndexOfByPredicate([&](const FInventoryEntry& Item)
	{
		return Item.ItemGuid == ItemGuid;
	});

	if (Index == INDEX_NONE)
		return false;

	FInventoryEntry& SourceItem = InventoryEntries.GetEntryByIndex(Index);

	if (SplitQuantity <= 0 || SplitQuantity >= SourceItem.Quantity)
		return false;

	// Capacity check: splitting creates a new stack
	if (MaxSlots > 0 && InventoryEntries.GetEntriesCount() >= MaxSlots)
	{
		UE_LOG(LogTemp, Log, TEXT("[InventoryComponent] SplitItemStackForDrag: no free slot for new stack"));
		return false;
	}

	// Reduce source quantity
	SourceItem.Quantity -= SplitQuantity;
	InventoryEntries.MarkItemDirty(SourceItem);
	PostInventoryItemChanged(SourceItem);

	// --- Create new instance for the new stack ---
	if (!SourceItem.ItemInstance || !SourceItem.ItemInstance->ItemDef)
	{
		return false;
	}
	const UInventoryItemDefinition* ItemDef = SourceItem.ItemInstance->ItemDef;

	UInventoryItemInstance* NewInstance = CreateItemInstance(ItemDef);
	if (!NewInstance)
	{
		return false;
	}

	InventoryEntries.AddItem(NewInstance, SplitQuantity);

	// The new stack is the last added
	if (InventoryEntries.GetEntriesCount() > 0)
	{
		const FInventoryEntry& NewItem = InventoryEntries.GetAllEntries().Last();
		OutNewStackGuid = NewItem.ItemGuid;

		UE_LOG(LogTemp, Log, TEXT("[InventoryComponent] SplitItemStackForDrag: Guid=%s Split=%d NewGuid=%s"),
			*ItemGuid.ToString(), SplitQuantity, *OutNewStackGuid.ToString());

		return true;
	}
	
	return false;
}

void UInventoryComponent::SplitAndMoveItem(const FGuid& SourceItemGuid, int32 SplitQuantity,
	UInventoryComponent* TargetInventory, int32 TargetSlotIndex)
{
	if (!TargetInventory || SplitQuantity <= 0) return;
	
	FGuid NewGuid;
	if (!SplitItemStackForDrag(SourceItemGuid, SplitQuantity, NewGuid)) return;
	
	// Same inventory
	if (TargetInventory == this)
	{
		MoveItemByGuid(NewGuid, TargetSlotIndex);
		return;
	}
	
	// Different inventory, try to move the new half to the target inventory
	const bool bMoveSuccess = MoveItemToInventory(TargetInventory, NewGuid, SplitQuantity, TargetSlotIndex);
	
	if (!bMoveSuccess)
	{
		// Move failed (target full, tag filter, etc.)
		// We must rollback the split so the player never "loses" items.

		TArray<FInventoryEntry>& Items = InventoryEntries.GetAllEntriesRef();

		// Find the newly created stack
		int32 NewIndex = Items.IndexOfByPredicate(
			[&NewGuid](const FInventoryEntry& Item)
			{
				return Item.ItemGuid == NewGuid;
			});

		// Find original stack (if still around)
		int32 SourceIndex = Items.IndexOfByPredicate(
			[&SourceItemGuid](const FInventoryEntry& Item)
			{
				return Item.ItemGuid == SourceItemGuid;
			});

		if (NewIndex != INDEX_NONE)
		{
			FInventoryEntry& NewItem = Items[NewIndex];

			if (SourceIndex != INDEX_NONE)
			{
				// Merge the split quantity back into the original stack
				FInventoryEntry& SourceItem = Items[SourceIndex];
				SourceItem.Quantity += NewItem.Quantity;
				InventoryEntries.MarkItemDirty(SourceItem);
				PostInventoryItemChanged(SourceItem);
			}
			else
			{
				// No original stack found (edge case). Just keep the new stack;
				// better to have extra items than to lose them.
				UE_LOG(LogTemp, Warning, TEXT("No original stack found."));
				return;
			}

			// Remove the temporary split stack
			const int32 RestoredQty = NewItem.Quantity;
			InventoryEntries.RemoveItem(NewGuid, RestoredQty); // will fire events
		}
	}
}

bool UInventoryComponent::MoveItemToInventory(UInventoryComponent* TargetInventory, const FGuid& ItemGuid,
                                              int32 Quantity, int32 TargetSlotIndex)
{
	if (!TargetInventory) return false;

	// -------- SOURCE LOOKUP --------
	TArray<FInventoryEntry>& SourceItems = InventoryEntries.GetAllEntriesRef();

	int32 SourceIndex = SourceItems.IndexOfByPredicate(
		[&ItemGuid](const FInventoryEntry& Item)
		{
			return Item.ItemGuid == ItemGuid;
		});

	if (SourceIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[InventoryComponent] MoveItemToInventory: Item not found (%s)"),
			*ItemGuid.ToString());
		return false;
	}

	FInventoryEntry& SourceItem = InventoryEntries.GetEntryByIndex(SourceIndex);

	if (!SourceItem.ItemInstance || !SourceItem.ItemInstance->ItemDef)
	{
		return false;
	}

	const UInventoryItemDefinition* ItemDef = SourceItem.ItemInstance->ItemDef;

	const int32 MoveQuantity = (Quantity <= 0 || Quantity > SourceItem.Quantity)
		? SourceItem.Quantity
		: Quantity;

	if (MoveQuantity <= 0)
	{
		return false;
	}

	// -------- NO TARGET SLOT SPECIFIED: old behavior --------
	if (TargetSlotIndex == INDEX_NONE)
	{
		const bool bFullyAdded = TargetInventory->AddItem(ItemDef, MoveQuantity);
		if (!bFullyAdded)
		{
			// Simple version: if not fully added, fail move (no partial moves)
			return false;
		}

		// Remove from this inventory
		InventoryEntries.RemoveItem(ItemGuid, MoveQuantity);

		return true;
	}

	// -------- TARGET SLOT SPECIFIED: honor where the user dropped --------

	// Clamp / validate TargetSlotIndex against target's MaxSlots
	if (TargetInventory->MaxSlots > 0)
	{
		if (TargetSlotIndex < 0 || TargetSlotIndex >= TargetInventory->MaxSlots)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[InventoryComponent] MoveItemToInventory: TargetSlotIndex %d out of range (MaxSlots=%d)"),
				TargetSlotIndex, TargetInventory->MaxSlots);
			return false;
		}
	}
	else
	{
		TargetSlotIndex = FMath::Max(0, TargetSlotIndex);
	}

	TArray<FInventoryEntry>& TargetItems = TargetInventory->InventoryEntries.GetAllEntriesRef();

	// Find if there's already an item in the target slot
	FInventoryEntry* TargetItem = TargetItems.FindByPredicate(
		[TargetSlotIndex](const FInventoryEntry& Item)
		{
			return Item.SlotIndex == TargetSlotIndex;
		});

	// Check stackable trait
	const UItemFragment_Stackable* StackableFragment =
		ItemDef->FindFragmentByClass<UItemFragment_Stackable>();

	const int32 MaxStackSize = StackableFragment
		? StackableFragment->GetMaxStackLimit()
		: 1;

	int32 RemainingToMove = MoveQuantity;

	// -----------------------------
	// 1) MERGE into existing target stack if compatible
	// -----------------------------
	if (TargetItem &&
		StackableFragment &&
		TargetItem->ItemInstance &&
		TargetItem->ItemInstance->ItemDef == ItemDef)
	{
		const int32 Space          = MaxStackSize - TargetItem->Quantity;
		const int32 TransferQty    = FMath::Min(Space, RemainingToMove);

		if (Space > 0 && TransferQty > 0)
		{
			TargetItem->Quantity += TransferQty;
			TargetInventory->InventoryEntries.MarkItemDirty(*TargetItem);
			TargetInventory->PostInventoryItemChanged(*TargetItem);

			// Remove moved quantity from the source stack
			InventoryEntries.RemoveItem(ItemGuid, TransferQty);

			UE_LOG(LogTemp, Log,
				TEXT("[InventoryComponent] MoveItemToInventory MERGE: %s -> TargetSlot=%d (+%d, now %d)"),
				*ItemGuid.ToString(), TargetSlotIndex, TransferQty, TargetItem->Quantity);

			// If we didn't move the full amount, leave the remainder in the source
			RemainingToMove -= TransferQty;
		}

		// We consider the operation "successful" if we moved at least something
		return (TransferQty > 0);
	}

	// -----------------------------
	// 2) TARGET SLOT TAKEN by incompatible item -> currently we fail
	//    (optional: implement cross-inventory swap here later)
	// -----------------------------
	if (TargetItem)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[InventoryComponent] MoveItemToInventory: Target slot %d occupied by different item, swap not implemented yet."),
			TargetSlotIndex);
		return false;
	}

	// -----------------------------
	// 3) EMPTY SLOT: create new stack at that exact slot
	// -----------------------------

	// Capacity check for creating a new stack
	if (TargetInventory->MaxSlots > 0 &&
		TargetInventory->InventoryEntries.GetEntriesCount() >= TargetInventory->MaxSlots)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[InventoryComponent] MoveItemToInventory: no free slot for new stack in target inventory"));
		return false;
	}

	// Create instance in target inventory
	UInventoryItemInstance* NewInstance = TargetInventory->CreateItemInstance(ItemDef);
	if (!NewInstance)
	{
		return false;
	}

	TargetInventory->InventoryEntries.AddItem(NewInstance, RemainingToMove, TargetSlotIndex);

	// Remove from this inventory
	InventoryEntries.RemoveItem(ItemGuid, RemainingToMove);

	UE_LOG(LogTemp, Log,
		TEXT("[InventoryComponent] MoveItemToInventory: Moved %d of %s -> TargetSlot=%d"),
		RemainingToMove, *ItemGuid.ToString(), TargetSlotIndex);

	return true;
}

bool UInventoryComponent::MoveItemByGuid(const FGuid& ItemGuid, int32 TargetSlotIndex)
{
	TArray<FInventoryEntry>& Items = InventoryEntries.GetAllEntriesRef();

	// Clamp target into legal slot range (0..MaxSlots-1) if MaxSlots > 0
	if (MaxSlots > 0)
	{
		if (TargetSlotIndex < 0 || TargetSlotIndex >= MaxSlots)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[InventoryComponent] MoveItemByGuid: TargetSlotIndex %d out of range (MaxSlots=%d)"),
				TargetSlotIndex, MaxSlots);
			return false;
		}
	}
	else
	{
		TargetSlotIndex = FMath::Max(0, TargetSlotIndex);
	}

	// Find the source item by Guid
	FInventoryEntry* SourceItem = Items.FindByPredicate(
		[&ItemGuid](const FInventoryEntry& Item)
		{
			return Item.ItemGuid == ItemGuid;
		});

	if (!SourceItem)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[InventoryComponent] MoveItemByGuid: Item not found (%s)"),
			*ItemGuid.ToString());
		return false;
	}

	if (SourceItem->SlotIndex == TargetSlotIndex)
	{
		// Same slot, nothing to do
		return false;
	}

	// Find if there's already an item in the target slot
	FInventoryEntry* TargetItem = Items.FindByPredicate(
		[TargetSlotIndex](const FInventoryEntry& Item)
		{
			return Item.SlotIndex == TargetSlotIndex;
		});

	// -----------------------------
	// 1) Try to MERGE into target stack
	// -----------------------------
	if (TargetItem &&
		SourceItem->ItemInstance && TargetItem->ItemInstance &&
		SourceItem->ItemInstance->ItemDef &&
		SourceItem->ItemInstance->ItemDef == TargetItem->ItemInstance->ItemDef)
	{
		const UInventoryItemDefinition* ItemDef = SourceItem->ItemInstance->ItemDef;

		// Check if this item type is stackable
		const UItemFragment_Stackable* StackableFragment =
			ItemDef->FindFragmentByClass<UItemFragment_Stackable>();

		if (StackableFragment)
		{
			const int32 MaxStackSize = StackableFragment->GetMaxStackLimit();
			const int32 CurrentTargetQty = TargetItem->Quantity;
			const int32 CurrentSourceQty = SourceItem->Quantity;

			const int32 Space = MaxStackSize - CurrentTargetQty;

			if (Space > 0 && CurrentSourceQty > 0)
			{
				// How much can we transfer from source to target?
				const int32 TransferQty = FMath::Min(Space, CurrentSourceQty);

				// 1. Increase target quantity
				TargetItem->Quantity += TransferQty;
				InventoryEntries.MarkItemDirty(*TargetItem);
				PostInventoryItemChanged(*TargetItem);

				// 2. Remove that amount from the source stack.
				//    This will either shrink it or completely remove it,
				//    and will correctly MarkArrayDirty / broadcast events.
				InventoryEntries.RemoveItem(SourceItem->ItemGuid, TransferQty);

				UE_LOG(LogTemp, Log,
					TEXT("[InventoryComponent] MoveItemByGuid MERGE: %s -> Slot=%d (+%d, now %d)"),
					*ItemGuid.ToString(), TargetSlotIndex, TransferQty, TargetItem->Quantity);

				OnInventoryRefreshed.Broadcast(Items);
				return true;
			}
		}
	}

	// -----------------------------
	// 2) Fallback: swap or move
	// -----------------------------
	if (TargetItem)
	{
		// Different item or can't merge: swap slot indices
		const int32 OldSourceSlot = SourceItem->SlotIndex;
		SourceItem->SlotIndex = TargetSlotIndex;
		TargetItem->SlotIndex = OldSourceSlot;

		InventoryEntries.MarkItemDirty(*SourceItem);
		InventoryEntries.MarkItemDirty(*TargetItem);
	}
	else
	{
		// Empty slot: just move the source
		SourceItem->SlotIndex = TargetSlotIndex;
		InventoryEntries.MarkItemDirty(*SourceItem);
	}

	UE_LOG(LogTemp, Log,
		TEXT("[InventoryComponent] MoveItemByGuid: Guid=%s -> Slot=%d"),
		*ItemGuid.ToString(), TargetSlotIndex);

	OnInventoryRefreshed.Broadcast(Items);
	return true;
}

bool UInventoryComponent::CanAcceptItemDefinition(const UInventoryItemDefinition* ItemDef) const
{
	if (!ItemDef)
	{
		return false;
	}

	// If no query configured, accept everything
	if (AllowedItemTagQuery.IsEmpty())
	{
		return true;
	}

	FGameplayTagContainer Tags;
	ItemDef->GetCombinedTags(Tags);

	return AllowedItemTagQuery.Matches(Tags);
}

void UInventoryComponent::GenerateLootFromTable(UInventoryLootTable* LootTable, int32 RandomSeed)
{
	if (!LootTable)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		// Only the server should generate loot; clients will get it via replication.
		return;
	}

	LootTable->GenerateLoot(this, RandomSeed);
}

void UInventoryComponent::PostInventoryItemAdded(const FInventoryEntry& Item)
{
	UE_LOG(LogTemp, Log, TEXT("[InventoryComponent] Item Added: %s"),
		*Item.GetDebugString());
	OnItemAdded.Broadcast(Item);
	
	// 🔹 Make sure all listeners (including widgets on clients) fully refresh
	OnInventoryRefreshed.Broadcast(InventoryEntries.GetAllEntries());
}

void UInventoryComponent::PostInventoryItemRemoved(const FInventoryEntry& Item)
{
	UE_LOG(LogTemp, Log, TEXT("[InventoryComponent] Item Removed: %s"),
		*Item.GetDebugString());
	OnItemRemoved.Broadcast(Item);
	
	// 🔹 Make sure all listeners (including widgets on clients) fully refresh
	OnInventoryRefreshed.Broadcast(InventoryEntries.GetAllEntries());
}

void UInventoryComponent::PostInventoryItemChanged(const FInventoryEntry& Item)
{
	UE_LOG(LogTemp, Log, TEXT("[InventoryComponent] Item Changed: %s"),
		*Item.GetDebugString());
	OnItemChanged.Broadcast(Item);
	
	// 🔹 Make sure all listeners (including widgets on clients) fully refresh
	OnInventoryRefreshed.Broadcast(InventoryEntries.GetAllEntries());
}

bool UInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FInventoryEntry& Entry : InventoryEntries.GetAllEntries())
	{
		if (!Entry.ItemInstance) continue;
		
		UInventoryItemInstance* Instance = Entry.ItemInstance.Get();
		if (Instance && IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void UInventoryComponent::ReadyForReplication()
{
	Super::ReadyForReplication();
	
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FInventoryEntry& Entry : InventoryEntries.GetAllEntries())
		{
			if (!Entry.ItemInstance) continue;
			
			UInventoryItemInstance* Instance = Entry.ItemInstance.Get();
			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

UInventoryItemInstance* UInventoryComponent::CreateItemInstance(const UInventoryItemDefinition* ItemDef)
{
	if (!ItemDef)
	{
		return nullptr;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	// Choose instance class (fallback to base instance if asset doesn’t specify)
	TSubclassOf<UInventoryItemInstance> InstanceClass;
	if (ItemDef->ItemInstanceClass)
		InstanceClass = ItemDef->ItemInstanceClass;
	else
		InstanceClass = UInventoryItemInstance::StaticClass();

	// Lyra uses actor as outer (UE-127172), do the same
	UInventoryItemInstance* NewInstance =
		NewObject<UInventoryItemInstance>(OwnerActor, InstanceClass);

	// Bind definition
	NewInstance->Initialize(ItemDef, OwnerActor);

	// Let fragments initialize per-instance data
	for (UInventoryItemFragment* Fragment : ItemDef->Fragments)
	{
		if (Fragment)
		{
			Fragment->OnInstanceCreated(NewInstance);
		}
	}

	// Register as replicated subobject if we’re using the subobject list
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		AddReplicatedSubObject(NewInstance);
	}

	return NewInstance;
}

void UInventoryComponent::OnRep_MaxSlots()
{
	///
}
