// Copyright Peter Gyarmati (BitroseStudio)


#include "Inventory/InventoryItemInstance.h"

#include "IDetailTreeNode.h"
#include "DataAssets/InventoryItemDefinition.h"
#include "Inventory/Fragments/InventoryItemFragment.h"
#include "Net/UnrealNetwork.h"

UInventoryItemInstance::UInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
{
}

void UInventoryItemInstance::Initialize(const UInventoryItemDefinition* InItemDef, AActor* InOwner)
{
	ItemDef = InItemDef;
	OwningActor = InOwner;
}

const UInventoryItemFragment* UInventoryItemInstance::FindFragmentByClass(
	TSubclassOf<UInventoryItemFragment> FragmentClass) const
{
	if (ItemDef && FragmentClass)
		return ItemDef->FindFragmentByClass(FragmentClass);
	return nullptr;
}

void UInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, ItemDef);
	DOREPLIFETIME(ThisClass, InstanceTags);
}
