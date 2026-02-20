#include "YIItemPickup.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "YIItemRegistrySubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "YIItemInstance.h"

AYIItemPickup::AYIItemPickup()
{
	bReplicates = true;
	bAlwaysRelevant = false;
	SetReplicateMovement(true);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
	MeshComponent->SetSimulatePhysics(true);

	// Fallback mesh for editor/testing
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CubeMesh.Object);
		MeshComponent->SetRelativeScale3D(FVector(0.4f));
	}

	ItemCode = 0;
	Count = 1;
}

void AYIItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AYIItemPickup, ItemCode);
	DOREPLIFETIME(AYIItemPickup, Count);
	DOREPLIFETIME(AYIItemPickup, ItemInstance);
}

void AYIItemPickup::SetItemByCode(int64 InCode, int32 InCount)
{
	if (!HasAuthority())
	{
		return;
	}
	ItemCode = InCode;
	Count = FMath::Max(1, InCount);
	ItemInstance.Definition = Definition;
	ItemInstance.Count = Count;
	if (!ItemInstance.InstanceId.IsValid())
	{
		ItemInstance.InstanceId = FGuid::NewGuid();
	}
	if (!ItemInstance.StackId.IsValid())
	{
		ItemInstance.StackId = FGuid::NewGuid();
	}
	OnRep_ItemData();
}

UYIItemDefinition* AYIItemPickup::GetItemDefinition() const
{
	if (Definition.IsValid())
	{
		return Definition.Get();
	}
	if (Definition.ToSoftObjectPath().IsValid())
	{
		return Definition.Get();
	}
	return nullptr;
}

void AYIItemPickup::OnRep_ItemData()
{
	RefreshVisuals();
}

void AYIItemPickup::RefreshVisuals()
{
	// If no code set and a designer selection exists, pick that (server only; clients rely on replication)
	if (HasAuthority() && ItemCode == 0 && SelectableDefinitions.IsValidIndex(SelectedDefinitionIndex))
	{
		if (UYIItemDefinition* Picked = SelectableDefinitions[SelectedDefinitionIndex].LoadSynchronous())
		{
			ItemCode = Picked->UniqueCode;
			Definition = Picked;
		}
	}

	UYIItemDefinition* Def = nullptr;

	if (!Def && ItemInstance.Definition.IsValid())
	{
		Def = ItemInstance.Definition.Get();
	}

	if (ItemCode != 0 && GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			Def = Registry->GetByCode(ItemCode);
		}
	}

	if (Def)
	{
		Definition = Def;
		ItemCode = Def->UniqueCode;
		if (ItemInstance.Definition != Definition)
		{
			ItemInstance.Definition = Definition;
		}

		if (const FInstancedStruct* PickupFragmentStruct = YIItemSchema::FindResolvedDefinitionFragmentByStruct(Def, FYIItemPickupDefinitionFragment::StaticStruct()))
		{
			if (const FYIItemPickupDefinitionFragment* PickupFragment = PickupFragmentStruct->GetPtr<FYIItemPickupDefinitionFragment>())
			{
				if (UStaticMesh* DropMesh = PickupFragment->WorldMesh.IsValid()
					? PickupFragment->WorldMesh.Get()
					: PickupFragment->WorldMesh.LoadSynchronous())
				{
					MeshComponent->SetStaticMesh(DropMesh);
				}

				MeshComponent->SetRelativeScale3D(PickupFragment->MeshScale);

				if (!PickupFragment->CollisionProfile.IsNone())
				{
					MeshComponent->SetCollisionProfileName(PickupFragment->CollisionProfile);
				}

				MeshComponent->SetSimulatePhysics(PickupFragment->bSimulatePhysicsOnDrop);
			}
		}
	}

	// Apply a lightweight color cue based on code for visual variety
	const uint32 Hash = GetTypeHash(ItemCode);
	const FLinearColor Tint(
		((Hash >> 0) & 0xFF) / 255.f,
		((Hash >> 8) & 0xFF) / 255.f,
		((Hash >> 16) & 0xFF) / 255.f,
		1.f);
	MeshComponent->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Tint));
}
