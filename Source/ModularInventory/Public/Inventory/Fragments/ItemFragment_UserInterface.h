// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "ItemFragment_UserInterface.generated.h"

/**
 * 
 */
UCLASS(DisplayName = "User Interface")
class MODULARINVENTORY_API UItemFragment_UserInterface : public UInventoryItemFragment
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Modular Inventory|User Interface Fragment")
	FText GetDisplayName() const { return DisplayName; }
	
	UFUNCTION(BlueprintPure, Category = "Modular Inventory|User Interface Fragment")
	FText GetDescription() const { return Description; }
	
	UFUNCTION(BlueprintPure, Category = "Modular Inventory|User Interface Fragment")
	UTexture2D* GetIcon() const { return Icon; }
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "User Interface")
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly, Category = "User Interface", meta = (MultiLine = "true"))
	FText Description;
	
	UPROPERTY(EditDefaultsOnly, Category = "User Interface")
	TObjectPtr<UTexture2D> Icon;
};
