// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryComponent.h"
#include "InventoryPanelWidget.generated.h"

class UInventorySlotWidget;
class UUniformGridPanel;
/**
 * Generic inventory panel bound to a single InventoryComponent.
 * Can be used for PlayerInventory, Hotbar, Storage, etc.
 */
UCLASS(Blueprintable)
class MODULARINVENTORY_API UInventoryPanelWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** The inventory this panel visualizes. Set via ExposeOnSpawn or from HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Modular Inventory|UI", meta=(ExposeOnSpawn="true"))
	TObjectPtr<UInventoryComponent> SourceInventory;

	/** Widget class used for each slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Modular Inventory|UI")
	TSubclassOf<UInventorySlotWidget> SlotWidgetClass;

	/** Panel that will contain slot widgets (e.g., UniformGridPanel, HorizontalBox, etc.). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Modular Inventory|UI")
	TObjectPtr<UUniformGridPanel> ItemsPanel;
	
	// UUserWidget
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Bind this panel to a specific inventory at runtime. */
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|UI")
	void InitializeWithInventory(UInventoryComponent* InInventory);

	/** Rebuild all slots from current inventory state. */
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|UI")
	void RebuildFromInventory();

protected:
	// Event handlers for inventory events
	UFUNCTION()
	void HandleItemAdded(const FInventoryEntry& Item);

	UFUNCTION()
	void HandleItemRemoved(const FInventoryEntry& Item);

	UFUNCTION()
	void HandleItemChanged(const FInventoryEntry& Item);

	UFUNCTION()
	void HandleInventoryRefreshed(const TArray<FInventoryEntry>& Items);

	UFUNCTION()
	void HandleMaxSlotsChanged(int32 NewMaxSlots);

	/** Optional: BP hook after we rebuilt the entire panel. */
	UFUNCTION(BlueprintImplementableEvent, Category="Modular Inventory|UI")
	void OnPanelRebuilt();
	
private:
	UPROPERTY(EditDefaultsOnly, Category="Modular Inventory|UI")
	int32 NumColumns = 5;
};
