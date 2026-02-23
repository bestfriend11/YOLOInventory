#include "YIWorldLootBlueprintLibrary.h"

#include "YIInventoryBag.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YIItemDefinition.h"
#include "YIItemInstance.h"
#include "YIItemInstanceFragmentAccess.h"
#include "YIItemPickup.h"
#include "YIItemRegistrySubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	static UWorld* YIWorldLoot_GetWorldFromContext(UObject* WorldContextObject)
	{
		return WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	}

	static UYIItemDefinition* YIWorldLoot_FindDefinitionByCode(int64 Code)
	{
		if (Code == 0 || !GEngine)
		{
			return nullptr;
		}
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			return Registry->GetByCode(Code);
		}
		return nullptr;
	}
}

AYIItemPickup* UYIWorldLootBlueprintLibrary::SpawnItemPickupByCode(UObject* WorldContextObject, int64 Code, const FTransform& Transform, int32 Count, TSubclassOf<AYIItemPickup> PickupClass)
{
	UWorld* World = YIWorldLoot_GetWorldFromContext(WorldContextObject);
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}

	UYIItemDefinition* Definition = YIWorldLoot_FindDefinitionByCode(Code);
	if (!Definition)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnItemPickupByCode: definition for code %lld not found."), static_cast<long long>(Code));
		return nullptr;
	}

	TSubclassOf<AYIItemPickup> ClassToSpawn = PickupClass ? PickupClass.Get() : AYIItemPickup::StaticClass();
	AYIItemPickup* Pickup = World->SpawnActorDeferred<AYIItemPickup>(ClassToSpawn, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Pickup)
	{
		Pickup->ItemCode = Code;
		Pickup->Count = FMath::Max(1, Count);
		UGameplayStatics::FinishSpawningActor(Pickup, Transform);
		Pickup->SetItemByCode(Code, Count);
	}
	return Pickup;
}

AYIItemPickup* UYIWorldLootBlueprintLibrary::SpawnItemPickup(UObject* WorldContextObject, UYIItemDefinition* Definition, const FTransform& Transform, int32 Count, TSubclassOf<AYIItemPickup> PickupClass)
{
	const int64 Code = Definition ? Definition->UniqueCode : 0;
	return SpawnItemPickupByCode(WorldContextObject, Code, Transform, Count, PickupClass);
}

AYIItemPickup* UYIWorldLootBlueprintLibrary::SpawnItemPickupFromInstance(UObject* WorldContextObject, const FYIItemInstance& Instance, const FTransform& Transform, TSubclassOf<AYIItemPickup> PickupClass)
{
	UWorld* World = YIWorldLoot_GetWorldFromContext(WorldContextObject);
	if (!World || World->GetNetMode() == NM_Client || Instance.Count <= 0)
	{
		return nullptr;
	}

	const FYIItemInstance& LocalInstance = Instance;

	int64 Code = 0;
	if (LocalInstance.Definition.IsValid())
	{
		Code = LocalInstance.Definition.Get()->UniqueCode;
	}
	else if (LocalInstance.Definition.ToSoftObjectPath().IsValid())
	{
		if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(LocalInstance.Definition.LoadSynchronous()))
		{
			Code = Def->UniqueCode;
		}
	}

	TSubclassOf<AYIItemPickup> ClassToSpawn = PickupClass ? PickupClass.Get() : AYIItemPickup::StaticClass();
	AYIItemPickup* Pickup = World->SpawnActorDeferred<AYIItemPickup>(ClassToSpawn, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Pickup)
	{
		Pickup->ItemCode = Code;
		Pickup->Count = LocalInstance.Count;
		Pickup->ItemInstance.Definition = LocalInstance.Definition;
		Pickup->ItemInstance.Count = LocalInstance.Count;
		Pickup->ItemInstance.InstanceId = LocalInstance.InstanceId.IsValid() ? LocalInstance.InstanceId : FGuid::NewGuid();
		Pickup->ItemInstance.StackId = LocalInstance.StackId.IsValid() ? LocalInstance.StackId : FGuid::NewGuid();
		Pickup->ItemInstance.CustomStackKey = LocalInstance.CustomStackKey;
		Pickup->ItemInstance.ContainedBagId = LocalInstance.ContainedBagId;
		Pickup->ItemInstance.bRotated = LocalInstance.bRotated;
		YIItemInstanceFragments::ExportNetFragmentPayload(LocalInstance, Pickup->ItemInstance.Fragments);
		UGameplayStatics::FinishSpawningActor(Pickup, Transform);
		Pickup->RefreshVisuals();
	}
	return Pickup;
}

bool UYIWorldLootBlueprintLibrary::DropBagItemToWorld(UObject* WorldContextObject, UYIInventoryBag* Bag, int32 Index, const FTransform& SpawnTransform, int32 Count, TSubclassOf<AYIItemPickup> PickupClass)
{
	UWorld* World = YIWorldLoot_GetWorldFromContext(WorldContextObject);
	if (!World || World->GetNetMode() == NM_Client || !Bag || !Bag->Items.IsValidIndex(Index))
	{
		return false;
	}

	FYIBagItem& Src = Bag->Items[Index];
	const int32 DropCount = (Count <= 0) ? Src.Item.Count : FMath::Min(Src.Item.Count, Count);
	if (DropCount <= 0)
	{
		return false;
	}

	UYIItemDefinition* Def = Src.Item.Definition.IsValid() ? Src.Item.Definition.Get() : Src.Item.Definition.LoadSynchronous();
	if (!Def)
	{
		return false;
	}

	FYIItemInstance DropInstance = Src.Item;
	DropInstance.Count = DropCount;

	AYIItemPickup* Pickup = SpawnItemPickupFromInstance(WorldContextObject, DropInstance, SpawnTransform, PickupClass);
	if (!Pickup)
	{
		return false;
	}

	if (DropCount < Src.Item.Count)
	{
		Src.Item.Count -= DropCount;
	}
	else
	{
		const FYIBagItem Removed = Src;
		Bag->Items.RemoveAt(Index);
		Bag->OnItemRemoved.Broadcast(Index, Removed);
	}
	Bag->OnChanged.Broadcast();
	return true;
}

bool UYIWorldLootBlueprintLibrary::DestroyBagItem(UObject* WorldContextObject, UYIInventoryBag* Bag, int32 Index, int32 Count)
{
	if (!Bag || !Bag->Items.IsValidIndex(Index))
	{
		return false;
	}
	if (UWorld* World = YIWorldLoot_GetWorldFromContext(WorldContextObject))
	{
		if (World->GetNetMode() == NM_Client)
		{
			return false;
		}
	}

	FYIBagItem& Src = Bag->Items[Index];
	const int32 DestroyCount = (Count <= 0) ? Src.Item.Count : FMath::Min(Src.Item.Count, Count);
	if (DestroyCount <= 0)
	{
		return false;
	}

	if (DestroyCount < Src.Item.Count)
	{
		Src.Item.Count -= DestroyCount;
	}
	else
	{
		const FYIBagItem Removed = Src;
		Bag->Items.RemoveAt(Index);
		Bag->OnItemRemoved.Broadcast(Index, Removed);
	}
	Bag->OnChanged.Broadcast();
	return true;
}

bool UYIWorldLootBlueprintLibrary::PickupItemActorIntoBag(UObject* WorldContextObject, UYIInventoryBag* Bag, AYIItemPickup* Pickup)
{
	UWorld* World = YIWorldLoot_GetWorldFromContext(WorldContextObject);
	if (!World || World->GetNetMode() == NM_Client || !Bag || !IsValid(Pickup))
	{
		return false;
	}

	const FYIItemInstanceNet& NetInstance = Pickup->ItemInstance;
	if (!NetInstance.Definition.IsValid() || NetInstance.Count <= 0)
	{
		return false;
	}

	FYIItemInstance Full;
	Full.Definition = NetInstance.Definition;
	Full.Count = NetInstance.Count;
	Full.InstanceId = NetInstance.InstanceId.IsValid() ? NetInstance.InstanceId : FGuid::NewGuid();
	Full.StackId = NetInstance.StackId.IsValid() ? NetInstance.StackId : FGuid::NewGuid();
	Full.CustomStackKey = NetInstance.CustomStackKey;
	Full.ContainedBagId = NetInstance.ContainedBagId;
	Full.bRotated = NetInstance.bRotated;
	YIItemInstanceFragments::ImportNetFragmentPayload(Full, NetInstance.Fragments);

	if (UYIInventoryBlueprintLibrary::AddItemInstanceToBag(Bag, Full))
	{
		Pickup->Destroy();
		Bag->OnChanged.Broadcast();
		return true;
	}
	return false;
}
