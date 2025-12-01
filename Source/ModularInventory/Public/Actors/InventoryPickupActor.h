// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryPickupActor.generated.h"

class UInventoryItemDefinition;
class UInventoryComponent;
class USphereComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UItemFragment_WorldMesh;

/**
 * World pickup actor for ModularInventory.
 * Holds a reference to an InventoryItemDefinition and a quantity.
 * On overlap, it tries to add itself to the overlapping actor's InventoryComponent.
 */
UCLASS()
class MODULARINVENTORY_API AInventoryPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AInventoryPickupActor();

protected:
	/** Simple root scene component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Modular Inventory|Pickup")
	TObjectPtr<USceneComponent> Root;

	/** Overlap volume used for pickup */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Modular Inventory|Pickup")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Modular Inventory|Pickup")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Modular Inventory|Pickup")
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	/** Item definition this pickup represents (PrimaryDataAsset) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ItemData,
			  Category="Modular Inventory|Pickup")
	TObjectPtr<const UInventoryItemDefinition> ItemDefinition;

	/** Quantity represented by this pickup */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ItemData,
			  Category="Modular Inventory|Pickup", meta=(ClampMin="1"))
	int32 Quantity;

	/** Rep callback: definition or quantity changed */
	UFUNCTION()
	void OnRep_ItemData();

	/** Overlap handler: only does work on the server */
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent,
	                   AActor* OtherActor,
	                   UPrimitiveComponent* OtherComp,
	                   int32 OtherBodyIndex,
	                   bool bFromSweep,
	                   const FHitResult& SweepResult);

	/** Server-side pickup logic */
	UFUNCTION(Server, Reliable)
	void ServerTryPickup(AActor* OverlappingActor);

	virtual void BeginPlay() override;
	
	/** Apply visual representation based on fragments on the ItemDef. */
	void RefreshVisualFromDefinition();

	/** Helper to configure mesh components from the UItemFragment_WorldMesh, if present. */
	void ApplyWorldMeshFragment(const UItemFragment_WorldMesh* WorldFrag);

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Initialize the pickup from an item definition + quantity (server only) */
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|Pickup")
	void InitializePickup(const UInventoryItemDefinition* InItemDef, int32 InQuantity);

	/** Definition getter */
	UFUNCTION(BlueprintPure, Category="Modular Inventory|Pickup")
	const UInventoryItemDefinition* GetItemDefinition() const { return ItemDefinition; }

	/** Quantity getter */
	UFUNCTION(BlueprintPure, Category="Modular Inventory|Pickup")
	int32 GetQuantity() const { return Quantity; }

	/**
	 * Called on clients whenever item data changes (OnRep & Initialize).
	 * Implement in BP to set mesh, floating text, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Modular Inventory|Pickup")
	void BP_OnPickupDataChanged();

	/**
	 * Convenience factory: spawn a pickup from a definition at runtime.
	 * Call this on the server.
	 */
	UFUNCTION(BlueprintCallable, Category="Modular Inventory|Pickup", meta=(WorldContext="WorldContextObject"))
	static AInventoryPickupActor* SpawnPickupFromDefinition(
		UObject* WorldContextObject,
		const UInventoryItemDefinition* ItemDef,
		int32 InQuantity,
		const FTransform& SpawnTransform);
};
