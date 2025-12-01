// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "InventoryLootTable.generated.h"

class UInventoryItemDefinition;
/**
 * One possible loot entry (a type of item that can appear in a container).
 */
USTRUCT(BlueprintType)
struct FLootItemEntry
{
	GENERATED_BODY()

	/** Which item can drop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot")
	TSoftObjectPtr<UInventoryItemDefinition> ItemDefinition;

	/** Inclusive min/max quantity when this entry is chosen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot", meta=(ClampMin="1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot", meta=(ClampMin="1"))
	int32 MaxQuantity = 1;

	/**
	 * Relative weight used in weighted random selection.
	 * Higher weight = more likely to be chosen.
	 * If all weights are equal, selection is uniform.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot", meta=(ClampMin="0.0"))
	float Weight = 1.0f;

	/** Optional tag query to restrict this entry (e.g. only in certain biomes or containers). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot")
	FGameplayTagQuery OptionalTagFilter;
};

/**
 * Loot table: describes all possible items and quantities for a container.
 */
UCLASS(BlueprintType)
class MODULARINVENTORY_API UInventoryLootTable : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Min and max number of rolls we will do on this table. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot", meta=(ClampMin="0"))
	int32 MinRolls = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot", meta=(ClampMin="0"))
	int32 MaxRolls = 3;

	/** All possible loot entries. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot")
	TArray<FLootItemEntry> Entries;

	/**
	 * Generate loot directly into the given inventory.
	 * NOTE: Should be called on the server (authority) only.
	 */
	UFUNCTION(BlueprintCallable, Category="Loot")
	void GenerateLoot(class UInventoryComponent* TargetInventory, int32 RandomSeed = 0) const;

protected:
	/** Pick a random entry using weights. Returns nullptr if nothing valid. */
	const FLootItemEntry* PickRandomEntryWeighted(FRandomStream& Rng,
	                                             UInventoryComponent* TargetInventory) const;
};
