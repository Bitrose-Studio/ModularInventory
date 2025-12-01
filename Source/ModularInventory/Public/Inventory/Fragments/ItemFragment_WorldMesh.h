// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "ItemFragment_WorldMesh.generated.h"

class UStaticMesh;
class USkeletalMesh;

/**
 * World representation for item pickups.
 * Lets you choose either a static or skeletal mesh + relative transform.
 */
UCLASS(DisplayName="World Mesh")
class MODULARINVENTORY_API UItemFragment_WorldMesh : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	/** Optional static mesh for world / pickup representation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Inventory|World Mesh")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	/** Optional skeletal mesh for world / pickup representation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Inventory|World Mesh")
	TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

	/** Relative offset applied to the mesh component on the pickup actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Inventory|World Mesh")
	FVector RelativeLocation = FVector::ZeroVector;

	/** Relative rotation applied to the mesh component on the pickup actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Inventory|World Mesh")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	/** Relative scale used for the mesh component on the pickup actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Inventory|World Mesh")
	FVector RelativeScale = FVector(1.0f, 1.0f, 1.0f);

	// Convenience accessors
	UFUNCTION(BlueprintPure, Category = "Modular Inventory|World Mesh")
	UStaticMesh* GetStaticMesh() const { return StaticMesh; }

	UFUNCTION(BlueprintPure, Category = "Modular Inventory|World Mesh")
	USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }

	UFUNCTION(BlueprintPure, Category = "Modular Inventory|World Mesh")
	FTransform GetRelativeTransform() const
	{
		return FTransform(RelativeRotation, RelativeLocation, RelativeScale);
	}

	// You can optionally override AddDynamicTags if you want tags like "World.HasMesh"
	// virtual void AddDynamicTags(FGameplayTagContainer& TagContainer) const override;
};
