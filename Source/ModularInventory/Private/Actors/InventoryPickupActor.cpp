// Copyright Peter Gyarmati (BitroseStudio)


#include "Actors/InventoryPickupActor.h"

#include "Components/SphereComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/Fragments/ItemFragment_WorldMesh.h"
#include "Inventory/Libraries/InventoryFunctionLibrary.h"
#include "Net/UnrealNetwork.h"


AInventoryPickupActor::AInventoryPickupActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	// Root
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Collision
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->SetupAttachment(RootComponent);
	Collision->SetSphereRadius(50.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Overlap);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AInventoryPickupActor::HandleOverlap);

	// Static mesh (default visible)
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComponent->SetIsReplicated(false); // visual only, driven by ItemDef

	// Skeletal mesh (default hidden)
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMeshComponent->SetupAttachment(RootComponent);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetIsReplicated(false);

	Quantity = 1;
	ItemDefinition = nullptr;
}

void AInventoryPickupActor::OnRep_ItemData()
{
	// Called on clients when ItemDefinition / Quantity changes
	BP_OnPickupDataChanged();
	RefreshVisualFromDefinition();
}

void AInventoryPickupActor::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	ServerTryPickup(OtherActor);
}

void AInventoryPickupActor::ServerTryPickup_Implementation(AActor* OverlappingActor)
{
	if (!ItemDefinition || Quantity <= 0 || !IsValid(OverlappingActor))
	{
		return;
	}

	// Find the player's inventory component
	UInventoryComponent* Inventory = OverlappingActor->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		return;
	}

	// Use your Blueprint-friendly helper that wraps AddItem()
	const bool bAdded = UInventoryFunctionLibrary::AddItemFromDefinition(
		Inventory, ItemDefinition, Quantity);

	if (bAdded)
	{
		// Inventory accepted the full quantity, destroy the pickup
		Destroy();
	}
	// If you want partial pickup, you could modify AddItemFromDefinition and
	// adjust Quantity here instead of just destroying.
}

void AInventoryPickupActor::BeginPlay()
{
	Super::BeginPlay();
	
	InitializePickup(ItemDefinition, Quantity);
}

void AInventoryPickupActor::RefreshVisualFromDefinition()
{
	if (!ItemDefinition)
	{
		// No definition → hide both meshes.
		if (StaticMeshComponent)   StaticMeshComponent->SetVisibility(false, true);
		if (SkeletalMeshComponent) SkeletalMeshComponent->SetVisibility(false, true);
		return;
	}

	// Look for our world mesh fragment on the definition
	const UItemFragment_WorldMesh* WorldFrag =
		Cast<UItemFragment_WorldMesh>(
			ItemDefinition->FindFragmentByClass(UItemFragment_WorldMesh::StaticClass())
		);

	ApplyWorldMeshFragment(WorldFrag);
}

void AInventoryPickupActor::ApplyWorldMeshFragment(const UItemFragment_WorldMesh* WorldFrag)
{
	if (!StaticMeshComponent || !SkeletalMeshComponent)
	{
		return;
	}

	if (!WorldFrag)
	{
		// No fragment: maybe keep whatever default visual the BP put on the actor
		// or you can choose to hide both:
		// StaticMeshComponent->SetVisibility(false, true);
		// SkeletalMeshComponent->SetVisibility(false, true);
		return;
	}

	const FTransform RelXform = WorldFrag->GetRelativeTransform();

	// Decide whether to use static or skeletal mesh
	UStaticMesh*   SM = WorldFrag->GetStaticMesh();
	USkeletalMesh* SK = WorldFrag->GetSkeletalMesh();

	if (SM)
	{
		StaticMeshComponent->SetStaticMesh(SM);
		StaticMeshComponent->SetRelativeTransform(RelXform);
		StaticMeshComponent->SetVisibility(true, true);

		SkeletalMeshComponent->SetVisibility(false, true);
		SkeletalMeshComponent->SetSkeletalMesh(nullptr);
	}
	else if (SK)
	{
		SkeletalMeshComponent->SetSkeletalMesh(SK);
		SkeletalMeshComponent->SetRelativeTransform(RelXform);
		SkeletalMeshComponent->SetVisibility(true, true);

		StaticMeshComponent->SetVisibility(false, true);
		StaticMeshComponent->SetStaticMesh(nullptr);
	}
	else
	{
		// Fragment exists but no meshes: hide both, or keep default.
		StaticMeshComponent->SetVisibility(false, true);
		SkeletalMeshComponent->SetVisibility(false, true);
	}
}

void AInventoryPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, ItemDefinition);
	DOREPLIFETIME(ThisClass, Quantity);
}

void AInventoryPickupActor::InitializePickup(const UInventoryItemDefinition* InItemDef, int32 InQuantity)
{
	// Only the server should own the authoritative item data
	if (!HasAuthority())
	{
		return;
	}

	ItemDefinition = InItemDef;
	Quantity       = FMath::Max(InQuantity, 1);

	// In the future you can grab a "WorldMesh" fragment here and set the mesh.

	// Notify blueprints on both server (immediately) and clients (via OnRep)
	BP_OnPickupDataChanged();
	RefreshVisualFromDefinition();
}

AInventoryPickupActor* AInventoryPickupActor::SpawnPickupFromDefinition(UObject* WorldContextObject,
	const UInventoryItemDefinition* ItemDef, int32 InQuantity, const FTransform& SpawnTransform)
{
	if (!WorldContextObject || !ItemDef || InQuantity <= 0)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AInventoryPickupActor* Pickup = World->SpawnActor<AInventoryPickupActor>(
		AInventoryPickupActor::StaticClass(),
		SpawnTransform,
		Params);

	if (Pickup && Pickup->HasAuthority())
	{
		Pickup->InitializePickup(ItemDef, InQuantity);
	}

	return Pickup;
}
