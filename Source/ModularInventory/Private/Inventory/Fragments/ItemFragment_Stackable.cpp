// Copyright Peter Gyarmati (BitroseStudio)


#include "Inventory/Fragments/ItemFragment_Stackable.h"
#include "Inventory/InventoryGameplayTags.h"


UItemFragment_Stackable::UItemFragment_Stackable()
{
	FragmentTags.AddTag(ItemTagTraitStackable);
}
