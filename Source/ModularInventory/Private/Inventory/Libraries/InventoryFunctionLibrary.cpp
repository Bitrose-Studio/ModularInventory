// Copyright Peter Gyarmati (BitroseStudio)


#include "Inventory/Libraries/InventoryFunctionLibrary.h"

#include "DataAssets/InventoryItemDefinition.h"
#include "Inventory/Fragments/InventoryItemFragment.h"

const UInventoryItemFragment* UInventoryFunctionLibrary::FindItemDefinitionFragmentByClass(
	const UInventoryItemDefinition* ItemDef, TSubclassOf<UInventoryItemFragment> FragmentClass)
{
	if (!ItemDef || !*FragmentClass) return nullptr;
	
	const UInventoryItemFragment* Fragment = ItemDef->FindFragmentByClass(FragmentClass);
	return Fragment;
}
