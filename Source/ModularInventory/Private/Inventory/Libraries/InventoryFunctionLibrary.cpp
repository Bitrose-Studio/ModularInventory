// Copyright Peter Gyarmati (BitroseStudio)


#include "Inventory/Libraries/InventoryFunctionLibrary.h"

#include "DataAssets/InventoryItemDefinition.h"
#include "Inventory/Fragments/InventoryItemFragment.h"
#include "Inventory/InventoryComponent.h"

const UInventoryItemFragment* UInventoryFunctionLibrary::FindItemDefinitionFragmentByClass(
	const UInventoryItemDefinition* ItemDef, TSubclassOf<UInventoryItemFragment> FragmentClass)
{
	if (!ItemDef || !*FragmentClass) return nullptr;
	
	const UInventoryItemFragment* Fragment = ItemDef->FindFragmentByClass(FragmentClass);
	return Fragment;
}

bool UInventoryFunctionLibrary::AddItemFromDefinition(UInventoryComponent* Inventory,
	const UInventoryItemDefinition* ItemDef, int32 Quantity)
{
	if (!Inventory || !ItemDef || Quantity <= 0)
	{
		return false;
	}

	return Inventory->AddItem(ItemDef, Quantity);
}
