// Copyright Peter Gyarmati (BitroseStudio)


#include "Inventory/Fragments/InventoryItemFragment.h"

void UInventoryItemFragment::AddDynamicTags_Implementation(FGameplayTagContainer& TagContainer) const
{
	if (!FragmentTags.IsEmpty())
		TagContainer.AppendTags(FragmentTags);
}
