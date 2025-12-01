// Copyright Peter Gyarmati (BitroseStudio)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryStorageActor.generated.h"

class UInventoryComponent;
class UInventoryLootTable;

UCLASS(BlueprintType)
class MODULARINVENTORY_API AInventoryStorageActor : public AActor
{
	GENERATED_BODY()

public:
	AInventoryStorageActor();

protected:
	virtual void BeginPlay() override;
	
private:
	UPROPERTY(EditAnywhere, Category="Loot")
	UInventoryLootTable* LootTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	UInventoryComponent* Inventory;

};
