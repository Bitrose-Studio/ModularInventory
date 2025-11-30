// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InventoryFunctionLibrary.generated.h"

class UInventoryItemDefinition;
class UInventoryItemFragment;
/**
 * 
 */
UCLASS()
class MODULARINVENTORY_API UInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Modular Inventory|Inventory Function Library", meta = (DeterminesOutputType=FragmentClass))
	static const UInventoryItemFragment* FindItemDefinitionFragmentByClass(
		const UInventoryItemDefinition* ItemDef,
		TSubclassOf<UInventoryItemFragment> FragmentClass);
};
