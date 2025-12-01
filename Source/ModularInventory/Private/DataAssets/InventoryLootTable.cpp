// Copyright Peter Gyarmati (BitroseStudio)


#include "DataAssets/InventoryLootTable.h"

#include "Inventory/InventoryComponent.h"

void UInventoryLootTable::GenerateLoot(class UInventoryComponent* TargetInventory, int32 RandomSeed) const
{
	if (!TargetInventory)
	{
		return;
	}

	// Only run loot generation on the server.
	AActor* OwnerActor = TargetInventory->GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	// No entries or no rolls – nothing to do.
	if (Entries.Num() == 0 || MaxRolls <= 0)
	{
		return;
	}

	FRandomStream Rng;
	if (RandomSeed != 0)
	{
		Rng.Initialize(RandomSeed);
	}
	else
	{
		Rng.Initialize(FMath::Rand());
	}

	const int32 ActualRolls = (MinRolls == MaxRolls)
		? MinRolls
		: Rng.RandRange(MinRolls, MaxRolls);

	for (int32 RollIndex = 0; RollIndex < ActualRolls; ++RollIndex)
	{
		const FLootItemEntry* Entry = PickRandomEntryWeighted(Rng, TargetInventory);
		if (!Entry || !Entry->ItemDefinition)
		{
			continue;
		}

		const int32 MinQ = FMath::Max(1, Entry->MinQuantity);
		const int32 MaxQ = FMath::Max(MinQ, Entry->MaxQuantity);

		const int32 Quantity = Rng.RandRange(MinQ, MaxQ);
		if (Quantity <= 0)
		{
			continue;
		}

		// Convert TSubclassOf -> CDO pointer for your AddItem(const UInventoryItemDefinition*, int32)
		const UInventoryItemDefinition* ItemDef = Entry->ItemDefinition.LoadSynchronous();
		if (!ItemDef)
		{
			continue;
		}

		// Let the inventory's own rules handle stacking, capacity, tag query, etc.
		TargetInventory->AddItem(ItemDef, Quantity);
	}
}

const FLootItemEntry* UInventoryLootTable::PickRandomEntryWeighted(FRandomStream& Rng,
	UInventoryComponent* TargetInventory) const
{
	// Build a list of valid entries + their weights.
	TArray<const FLootItemEntry*> ValidEntries;
	ValidEntries.Reserve(Entries.Num());
	TArray<float> Weights;
	Weights.Reserve(Entries.Num());

	for (const FLootItemEntry& Entry : Entries)
	{
		if (!Entry.ItemDefinition)
		{
			continue;
		}

		// Optional: apply tag filter against the target inventory's container type/tags
		if (!Entry.OptionalTagFilter.IsEmpty())
		{
			// Example: you might feed some tags representing environment or container type.
			// For now we skip this (no external tag source), but wiring it later is easy.
		}

		if (Entry.Weight <= 0.f)
		{
			continue;
		}

		ValidEntries.Add(&Entry);
		Weights.Add(Entry.Weight);
	}

	if (ValidEntries.Num() == 0)
	{
		return nullptr;
	}

	// Weighted random
	float TotalWeight = 0.f;
	for (float W : Weights)
	{
		TotalWeight += W;
	}

	if (TotalWeight <= 0.f)
	{
		return nullptr;
	}

	const float Pick = Rng.FRandRange(0.f, TotalWeight);
	float Accum = 0.f;

	for (int32 i = 0; i < ValidEntries.Num(); ++i)
	{
		Accum += Weights[i];
		if (Pick <= Accum)
		{
			return ValidEntries[i];
		}
	}

	// Fallback (floating point edge case)
	return ValidEntries.Last();
}
