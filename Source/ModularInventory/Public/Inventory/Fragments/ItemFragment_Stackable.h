// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "ItemFragment_Stackable.generated.h"

/**
 * 
 */
UCLASS(DisplayName = "Stackable")
class MODULARINVENTORY_API UItemFragment_Stackable : public UInventoryItemFragment
{
	GENERATED_BODY()
	
public:
	UItemFragment_Stackable();
	
	UFUNCTION(BlueprintPure, Category = "Modular Inventory|Stackable Fragment")
	int32 GetMaxStackLimit() const { return MaxStackLimit; }
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Stackable")
	int32 MaxStackLimit = 1;
};
