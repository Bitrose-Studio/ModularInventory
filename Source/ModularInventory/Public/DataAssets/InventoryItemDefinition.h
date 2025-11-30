// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "InventoryItemDefinition.generated.h"

class UInventoryItemFragment;
/**
 * 
 */
UCLASS(BlueprintType, Const)
class MODULARINVENTORY_API UInventoryItemDefinition : public UPrimaryDataAsset, public IGameplayTagAssetInterface
{
	GENERATED_BODY()
	
public:
	// Internal technical name (for debugging)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Inventory|Item Definition")
	FText DisplayName;
	
	// Static tags - e.g., Item.Weapon.Rifle
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Inventory|Item Definition")
	FGameplayTagContainer GameplayTags;

	// Cache of dynamic tags from fragments TODO: Do we need this?
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Modular Inventory|Item Definition")
	FGameplayTagContainer DynamicTags;
	
	// Runtime class for item instances
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Inventory|Item Definition")
	TSubclassOf<class UInventoryItemInstance> ItemInstanceClass;
	
	// Item fragments for behavior composition
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Inventory|Item Definition")
	TArray<TObjectPtr<UInventoryItemFragment>> Fragments;
	
	const UInventoryItemFragment* FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const;
	
	template <typename FragmentType>
	const FragmentType* FindFragmentByClass() const
	{
		return Cast<FragmentType>(FindFragmentByClass(FragmentType::StaticClass()));
	}
	
	// Call this in editor / on load to refresh the cache
	void RebuildDynamicTags();

	// Get GameplayTags & DynamicTags
	void GetCombinedTags(FGameplayTagContainer& OutTags) const;
	
	//~IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	//~End IGameplayTagAssetInterface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
