// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InventoryItemInstance.h"
#include "Components/ActorComponent.h"
#include "DataAssets/InventoryItemDefinition.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryComponent.generated.h"


UENUM(BlueprintType)
enum class EInventoryContainerType : uint8
{
	Generic			UMETA(DisplayName = "Generic"),
	PlayerInventory	UMETA(DisplayName = "PlayerInventory"),
	Hotbar			UMETA(DisplayName = "Hotbar"),
	Storage			UMETA(DisplayName = "Storage"),
};

/**
 * A single entry in an inventory
 */
USTRUCT(BlueprintType)
struct FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	FInventoryEntry() {}
	
	void PreReplicatedRemove(const struct FInventoryList& Serializer);
	void PostReplicatedAdd(const struct FInventoryList& Serializer);
	void PostReplicatedChange(const struct FInventoryList& Serializer);

	/**
	 * Helper functions for UI
	 */
	bool IsItemInstanceValid() const { return ItemInstance.IsValid(); }
	
	UInventoryItemInstance* GetItemInstance() const;
	int32 GetQuantity() const { return Quantity; }
	FGuid GetItemGuid() const { return ItemGuid; }
	int32 GetSlotIndex() const { return SlotIndex; }
	
	/**
	 * Debug
	 */
	FString GetDebugString() const
	{
		FString DefName = ItemInstance.IsValid() && ItemInstance->ItemDef
			? ItemInstance->ItemDef->GetName()
			: TEXT("None");
		return FString::Printf(TEXT("Item=%s Qty=%d Guid=%s"),
			*DefName, Quantity, *ItemGuid.ToString());
	}
	
private:
	friend struct FInventoryList;
	friend class UInventoryComponent;
	
	UPROPERTY(BlueprintReadOnly, Category = "Modular Inventory|Inventory Entry", meta = (AllowPrivateAccess = true))
	TWeakObjectPtr<UInventoryItemInstance> ItemInstance = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Modular Inventory|Inventory Entry", meta = (AllowPrivateAccess = true))
	int32 Quantity = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Modular Inventory|Inventory Entry", meta = (AllowPrivateAccess = true))
	FGuid ItemGuid = FGuid();
	UPROPERTY(BlueprintReadOnly, Category = "Modular Inventory|Inventory Entry", meta = (AllowPrivateAccess = true))
	int32 SlotIndex = INDEX_NONE;
};

/**
 * List of inventory items
 */
USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer 
{
	GENERATED_BODY()
	
	FInventoryList() : OwnerComponent(nullptr) {}
	FInventoryList(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) {}
	
	void SetOwnerComponent(UActorComponent* InOwnerComponent) { OwnerComponent = InOwnerComponent; }
	
	TArray<UInventoryItemInstance*> GetAllItems() const;
	
	TArray<FInventoryEntry> GetAllEntries() const { return Entries; }
	
	TArray<FInventoryEntry>& GetAllEntriesRef() { return Entries; }
	
	int32 GetEntriesCount() const { return Entries.Num(); }
	
	FInventoryEntry& GetEntryByIndex(const int32 Index) { return Entries[Index]; };
	
	// FFastArraySerializer contract
	// Called before removing elements and after the elements themselves are notified.  The indices are valid for this function call only! 
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	// Called after adding all new elements and after the elements themselves are notified.  The indices are valid for this function call only!
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	// Called after updating all existing elements with new data and after the elements themselves are notified. The indices are valid for this function call only!
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	// End FFastArraySerializer contract
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(Entries, DeltaParms, *this);
	}
	
	void AddItem(UInventoryItemInstance* Instance, int32 Quantity);
	bool RemoveItem(const FGuid& ItemGuid, int32 QuantityToRemove);
	
private:
	friend FInventoryEntry;
	
	// Replicated list of items
	UPROPERTY()
	TArray<FInventoryEntry> Entries;
	
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemChangeSignature, const FInventoryEntry&, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryRefreshedSignature, const TArray<FInventoryEntry>&, Entries);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryMaxSlotsChangedSignature, int32, NewMaxSlots);

UCLASS(Blueprintable, ClassGroup=(ModularInventory), meta=(BlueprintSpawnableComponent))
class MODULARINVENTORY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UInventoryComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	int32 FindFirstFreeSlotIndex() const;
	
	/** Set the maximum number of slots (stacks) this container can hold. 0 = unlimited. */
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|Inventory")
	void SetMaxSlots(int32 NewMaxSlots);

	/** Get current max slots. */
	UFUNCTION(BlueprintPure, Category="Modular Inventory|Inventory")
	int32 GetMaxSlots() const { return MaxSlots; }

	/** Returns how many more stacks could be created (ignores stacking into existing stacks). */
	UFUNCTION(BlueprintPure, Category="Modular Inventory|Inventory")
	int32 GetFreeSlotCount() const;
	
	UFUNCTION(BlueprintPure, Category="Modular Inventory|Inventory", meta = (DisplayName = "Find Item by Guid"))
	bool FindItemByGuid(const FGuid& ItemGuid, FInventoryEntry& OutItem) const;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modular Inventory|Inventory")
	void TryAddItem(const UInventoryItemDefinition* ItemDef);
	
	UFUNCTION(BlueprintCallable, Category = "Modular Inventory|Inventory", meta = (DisplayName = "Add Item", AllowedClasses = "InventoryItemDefinition"))
	bool AddItem(const UInventoryItemDefinition* ItemDef, int32 Quantity);
	
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|Inventory")
	bool RemoveItem(const FGuid& ItemGuid, int32 QuantityToRemove);
	
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|Inventory")
	bool SwapItems(int32 SlotIndexA, int32 SlotIndexB);
	
	UFUNCTION(BlueprintCallable)
	bool SplitItemStack(const FGuid& ItemGuid, int32 SplitQuantity);
	
	bool SplitItemStackForDrag(const FGuid& ItemGuid, int32 SplitQuantity, FGuid& OutNewStackGuid);
	
	// Temporary
	void SplitAndMoveItem(const FGuid& SourceItemGuid, int32 SplitQuantity, UInventoryComponent* TargetInventory, int32 TargetSlotIndex);
	
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|Inventory")
	bool MoveItemToInventory(UInventoryComponent* TargetInventory, const FGuid& ItemGuid, int32 Quantity = -1);
	
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|Inventory")
	bool MoveItemByGuid(const FGuid& ItemGuid, int32 TargetSlotIndex);
	
	/** Checks if this container can accept the given definition (tag filter only, not capacity). */
	UFUNCTION(BlueprintPure, Category="Modular Inventory|Inventory")
	bool CanAcceptItemDefinition(const UInventoryItemDefinition* ItemDef) const;

	// Called from FInventoryList
	void PostInventoryItemAdded(const FInventoryEntry& Item);
	void PostInventoryItemRemoved(const FInventoryEntry& Item);
	void PostInventoryItemChanged(const FInventoryEntry& Item);
	
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;
	
	/**
	 * Events
	 **/
	
	/** Fired when a new item stack is created (or added by replication). */
	UPROPERTY(BlueprintAssignable, Category = "Modular Inventory|Events")
	FInventoryItemChangeSignature OnItemAdded;
	/** Fired when an item stack is removed (or removed by replication). */
	UPROPERTY(BlueprintAssignable, Category = "Modular Inventory|Events")
	FInventoryItemChangeSignature OnItemRemoved;
	/** Fired when an existing stack changes (quantity, etc.). */
	UPROPERTY(BlueprintAssignable, Category = "Modular Inventory|Events")
	FInventoryItemChangeSignature OnItemChanged;
	/** Fired when the whole inventory should be refreshed (optional use). */
	UPROPERTY(BlueprintAssignable, Category="Modular Inventory|Events")
	FInventoryRefreshedSignature OnInventoryRefreshed;
	/** Fired when MaxSlots changes (e.g., backpack equipped). */
	UPROPERTY(BlueprintAssignable, Category="Modular Inventory|Events")
	FInventoryMaxSlotsChangedSignature OnMaxSlotsChanged;
	
	UPROPERTY(Replicated)
	FInventoryList InventoryEntries;
	
	/** Logical type of this container: player inventory, hotbar, storage, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modular Inventory|Config")
	EInventoryContainerType ContainerType = EInventoryContainerType::Generic;
	
	/** Maximum number of stacks (slots) this container can hold. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MaxSlots, Category="Modular Inventory|Config", meta=(ClampMin="1"))
	int32 MaxSlots = 5;
	
	/**
	 * Optional tag query describing which items are allowed in this container.
	 * Leave empty to allow all items.
	 * Example for hotbar: query that matches (Weapon OR Tool) categories.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modular Inventory|Config")
	FGameplayTagQuery AllowedItemTagQuery;
	
protected:
	UInventoryItemInstance* CreateItemInstance(const UInventoryItemDefinition* ItemDef);
	
	UFUNCTION()
	void OnRep_MaxSlots();
};
