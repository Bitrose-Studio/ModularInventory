// Copyright Peter Gyarmati (BitroseStudio)


#include "Actors/InventoryStorageActor.h"

#include "Inventory/InventoryComponent.h"


AInventoryStorageActor::AInventoryStorageActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	SetReplicates(true);
	
	Inventory = CreateDefaultSubobject<UInventoryComponent>("Inventory");
	Inventory->SetMaxSlots(8);
}

void AInventoryStorageActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		Inventory->GenerateLootFromTable(LootTable);
	}
}

