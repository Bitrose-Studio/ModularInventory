// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryDragVisualWidget.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class MODULARINVENTORY_API UInventoryDragVisualWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|UI")
	TObjectPtr<UTexture2D> DragIcon;
	
	UPROPERTY(BlueprintReadOnly, Category="Modular Inventory|UI")
	int32 DragQuantity;
};
