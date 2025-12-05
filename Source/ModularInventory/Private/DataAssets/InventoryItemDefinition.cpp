// Copyright Peter Gyarmati (BitroseStudio)


#include "DataAssets/InventoryItemDefinition.h"

#include "Inventory/Fragments/InventoryItemFragment.h"

const UInventoryItemFragment* UInventoryItemDefinition::FindFragmentByClass(
	TSubclassOf<UInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
				return Fragment;
		}
	}
	return nullptr;
}

void UInventoryItemDefinition::RebuildDynamicTags()
{
	DynamicTags.Reset();
	DynamicTags = GameplayTags;
	for (const UInventoryItemFragment* Fragment : Fragments)
	{
		if (!Fragment) continue;
		Fragment->AddDynamicTags(DynamicTags);
	}
}

void UInventoryItemDefinition::GetCombinedTags(FGameplayTagContainer& OutTags) const
{
	OutTags = GameplayTags;
	OutTags.AppendTags(DynamicTags);
}

void UInventoryItemDefinition::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	GetCombinedTags(TagContainer);
}

#if WITH_EDITOR
void UInventoryItemDefinition::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	static const FName FragmentsName(TEXT("Fragments"));
	if (PropertyChangedEvent.GetPropertyName() == FragmentsName ||
		PropertyChangedEvent.GetMemberPropertyName() == FragmentsName)
	{
		RebuildDynamicTags();
		//return;
	}

	// Also: maybe always rebuild on any change, it's cheap.
	RebuildDynamicTags();
}
#endif