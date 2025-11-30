// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "InventoryItemFragment.generated.h"

class UInventoryItemInstance;
/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class MODULARINVENTORY_API UInventoryItemFragment : public UObject
{
	GENERATED_BODY()
	
public:
	virtual void OnInstanceCreated(UInventoryItemInstance* InInstance) const {}
	
	// Override in child fragments to add their tags to the definition cache
	UFUNCTION(BlueprintNativeEvent, Category = "Item")
	void AddDynamicTags(FGameplayTagContainer& TagContainer) const;

	
protected:
	UPROPERTY(EditInstanceOnly, Category = "Fragment", meta = (DisplayName = "Fragment Tags"))
	FGameplayTagContainer FragmentTags;
};
