// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "InventoryItemInstance.generated.h"

class UInventoryItemFragment;
class UInventoryItemDefinition;
/**
 * 
 */
UCLASS(BlueprintType)
class MODULARINVENTORY_API UInventoryItemInstance : public UObject
{
	GENERATED_BODY()
	
public:
	UInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Allow this UObject to replicate as a subobject (Lyra pattern)
	virtual bool IsSupportedForNetworking() const override { return true; }

	/** The definition asset this instance is based on. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Modular Inventory|Item")
	TObjectPtr<const UInventoryItemDefinition> ItemDef = nullptr;

	/** Optional per-instance tags: durability state, flags, etc. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Modular Inventory|Item")
	FGameplayTagContainer InstanceTags;

	/** Actor that owns this instance (player, container, etc.). */
	UPROPERTY()
	TObjectPtr<AActor> OwningActor = nullptr;

	/** Initialize this instance from a definition + owning actor. */
	virtual void Initialize(const UInventoryItemDefinition* InItemDef, AActor* InOwner);

	UFUNCTION(BlueprintPure, Category="Modular Inventory|Item")
	AActor* GetOwningActor() const { return OwningActor; }

	UFUNCTION(BlueprintPure, Category="Modular Inventory|Item")
	const UInventoryItemDefinition* GetItemDef() const { return ItemDef; }

	/** Query fragments through the definition (Lyra-style). */
	UFUNCTION(BlueprintCallable, BlueprintPure=false,
			  Category="Modular Inventory|Item", meta=(DeterminesOutputType=FragmentClass))
	const UInventoryItemFragment* FindFragmentByClass(
		TSubclassOf<UInventoryItemFragment> FragmentClass) const;

	template<typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return Cast<ResultClass>(FindFragmentByClass(ResultClass::StaticClass()));
	}


protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
