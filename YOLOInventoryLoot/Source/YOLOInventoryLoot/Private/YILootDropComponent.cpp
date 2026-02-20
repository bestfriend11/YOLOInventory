#include "YILootDropComponent.h"
#include "YILootDropProfile.h"
#include "YIItemGenerator.h"
#include "YILootTable.h"
#include "YIItemDefinition.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIWorldLootBlueprintLibrary.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"
#include "Misc/DateTime.h"

namespace
{
static int32 YILootDrop_GetActorLevel(const AActor* Actor)
{
	if (!Actor)
	{
		return 1;
	}

	static const FName LevelNames[] = {
		TEXT("Level"),
		TEXT("CharacterLevel"),
		TEXT("ItemLevel")
	};

	for (const FName LevelName : LevelNames)
	{
		if (const FIntProperty* IntProp = FindFProperty<FIntProperty>(Actor->GetClass(), LevelName))
		{
			return FMath::Max(1, IntProp->GetPropertyValue_InContainer(Actor));
		}
	}

	return 1;
}
}

UYILootDropComponent::UYILootDropComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UYILootDropComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority() || !bAutoBindOwnerDestroyed)
	{
		return;
	}

	if (UYILootDropProfile* Profile = DropProfile.IsValid() ? DropProfile.Get() : DropProfile.LoadSynchronous())
	{
		if (Profile->bDropOnOwnerDestroyed)
		{
			GetOwner()->OnDestroyed.AddUniqueDynamic(this, &UYILootDropComponent::HandleOwnerDestroyed);
		}
	}
}

void UYILootDropComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYILootDropComponent, bHasDropped);
}

FYILootDropResult UYILootDropComponent::TriggerDrop()
{
	FYILootDropContext Context;
	return TriggerDropWithContext(Context);
}

FYILootDropResult UYILootDropComponent::TriggerDropForInstigator(AActor* InstigatorActor, int32 ContextLevel, int32 Seed)
{
	FYILootDropContext Context;
	Context.InstigatorActor = InstigatorActor;
	Context.ContextLevel = FMath::Max(1, ContextLevel);
	Context.Seed = Seed;
	return TriggerDropWithContext(Context);
}

FYILootDropResult UYILootDropComponent::TriggerDropWithContext(const FYILootDropContext& Context)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		ServerTriggerDropWithContext(Context);

		FYILootDropResult Pending;
		Pending.bSuccess = false;
		Pending.Message = FText::FromString(TEXT("Drop requested on server."));
		return Pending;
	}

	FYILootDropResult Result = TriggerDropInternal(Context);
	OnDropExecuted.Broadcast(Result);
	return Result;
}

void UYILootDropComponent::ServerTriggerDropWithContext_Implementation(const FYILootDropContext& Context)
{
	const FYILootDropResult Result = TriggerDropInternal(Context);
	OnDropExecuted.Broadcast(Result);
}

void UYILootDropComponent::ResetOneShotState()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	bHasDropped = false;
}

void UYILootDropComponent::HandleOwnerDestroyed(AActor* DestroyedActor)
{
	if (!DestroyedActor || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FYILootDropContext Context;
	TriggerDropInternal(Context);
}

void UYILootDropComponent::OnRep_HasDropped()
{
}

FYILootDropResult UYILootDropComponent::TriggerDropInternal(const FYILootDropContext& Context)
{
	FYILootDropResult Result;

	UYILootDropProfile* Profile = DropProfile.IsValid() ? DropProfile.Get() : DropProfile.LoadSynchronous();
	if (!Profile)
	{
		Result.Message = FText::FromString(TEXT("Drop profile is missing."));
		return Result;
	}

	if (Profile->bOneShot && bHasDropped)
	{
		Result.Message = FText::FromString(TEXT("Drop already consumed (one-shot profile)."));
		return Result;
	}

	int32 Level = 1;
	switch (Profile->LevelSource)
	{
	case EYILootDropLevelSource::FixedLevel:
		Level = FMath::Max(1, Profile->FixedLevel);
		break;
	case EYILootDropLevelSource::OwnerLevel:
		Level = YILootDrop_GetActorLevel(GetOwner());
		break;
	case EYILootDropLevelSource::InstigatorLevel:
		Level = YILootDrop_GetActorLevel(Context.InstigatorActor);
		break;
	case EYILootDropLevelSource::ContextLevel:
	default:
		Level = FMath::Max(1, Context.ContextLevel);
		break;
	}

	Level += Profile->LevelOffset;
	Level = FMath::Max(1, Level);
	if (Profile->bClampLevel)
	{
		const int32 MinLevel = FMath::Max(1, Profile->MinLevel);
		const int32 MaxLevel = FMath::Max(MinLevel, Profile->MaxLevel);
		Level = FMath::Clamp(Level, MinLevel, MaxLevel);
	}
	Result.EffectiveLevel = Level;

	int32 Seed = Context.Seed != 0 ? Context.Seed : Profile->DefaultSeed;
	if (Seed == 0)
	{
		Seed = (int32)(FDateTime::UtcNow().GetTicks() & 0x7FFFFFFF);
		Seed = HashCombineFast(Seed, GetOwner() ? GetOwner()->GetUniqueID() : 0);
		if (Context.InstigatorActor)
		{
			Seed = HashCombineFast(Seed, Context.InstigatorActor->GetUniqueID());
		}
	}
	if (Seed == 0)
	{
		Seed = 1;
	}
	Result.EffectiveSeed = Seed;
	FRandomStream RNG(Seed);

	auto ResolveInventory = [this, &Context, Profile]() -> UYIInventoryComponent*
	{
		if (Context.TargetInventory)
		{
			return Context.TargetInventory;
		}

		if (Profile->bPreferInstigatorInventory && Context.InstigatorActor)
		{
			if (UYIInventoryComponent* InstigatorInv = Context.InstigatorActor->FindComponentByClass<UYIInventoryComponent>())
			{
				return InstigatorInv;
			}
		}

		if (Profile->bFallbackToOwnerInventory && GetOwner())
		{
			if (UYIInventoryComponent* OwnerInv = GetOwner()->FindComponentByClass<UYIInventoryComponent>())
			{
				return OwnerInv;
			}
		}

		return nullptr;
	};

	auto GetSpawnTransform = [this, &Context, Profile, &RNG]() -> FTransform
	{
		FTransform SpawnTransform = Context.bOverrideSpawnTransform
			? Context.SpawnTransform
			: (GetOwner() ? GetOwner()->GetActorTransform() : FTransform::Identity);

		if (Profile->PickupScatterRadius > 0.0f)
		{
			const float Radius = Profile->PickupScatterRadius;
			const FVector Offset(RNG.FRandRange(-Radius, Radius), RNG.FRandRange(-Radius, Radius), 0.0f);
			SpawnTransform.AddToTranslation(Offset);
		}
		return SpawnTransform;
	};

	auto RouteItem = [this, &Context, Profile, &Result, &ResolveInventory, &GetSpawnTransform](const FYIBagItem& Item) -> bool
	{
		if (!Item.Item.Definition.ToSoftObjectPath().IsValid() || Item.Item.Count <= 0)
		{
			return false;
		}

		const bool bDirectInventory = (Profile->SpawnMode == EYILootDropSpawnMode::DirectToInventory);
		if (bDirectInventory)
		{
			UYIInventoryComponent* Inventory = ResolveInventory();
			UYIInventoryBag* TargetBag = Context.TargetBag;
			if (!TargetBag && Inventory)
			{
				TargetBag = Inventory->GetBag();
			}

			if (TargetBag)
			{
				bool bAdded = false;
				if (Inventory && TargetBag == Inventory->GetBag())
				{
					bAdded = (Inventory->AddBagItem(Item) != INDEX_NONE);
				}
				else
				{
					bAdded = (TargetBag->AddBagItem(Item) != INDEX_NONE);
					if (bAdded && Inventory && TargetBag == Inventory->GetBag())
					{
						Inventory->SyncNetState();
					}
				}

				if (bAdded)
				{
					++Result.NumAddedToInventory;
					return true;
				}
			}

			if (!Profile->bFallbackToWorldPickup)
			{
				return false;
			}
		}

		const FTransform SpawnTransform = GetSpawnTransform();
		AYIItemPickup* Pickup = UYIWorldLootBlueprintLibrary::SpawnItemPickupFromInstance(
			this,
			Item.Item,
			SpawnTransform,
			Profile->PickupClassOverride);

		if (Pickup)
		{
			++Result.NumSpawnedPickups;
			return true;
		}

		return false;
	};

	// Guaranteed drops first.
	for (const FYILootGuaranteedDropEntry& Guaranteed : Profile->GuaranteedDrops)
	{
		const float Chance = FMath::Clamp(Guaranteed.Chance, 0.0f, 1.0f);
		if (Chance <= 0.0f || RNG.FRand() > Chance)
		{
			continue;
		}

		UYIItemDefinition* Def = Guaranteed.Definition.IsValid()
			? Guaranteed.Definition.Get()
			: Guaranteed.Definition.LoadSynchronous();
		if (!Def)
		{
			continue;
		}

		const int32 MinCount = FMath::Max(1, Guaranteed.MinCount);
		const int32 MaxCount = FMath::Max(MinCount, Guaranteed.MaxCount);
		const int32 Count = (MaxCount > MinCount) ? RNG.RandRange(MinCount, MaxCount) : MinCount;

		FYIBagItem Item;
		Item.Item.Definition = Def;
		Item.Item.Count = Count;
		Item.Size = Def->GetEffectiveDefaultSize();
		Item.Pos = FIntPoint::ZeroValue;

		if (RouteItem(Item))
		{
			++Result.NumGuaranteedDrops;
		}
	}

	// Generated rolls.
	UYIItemGenerator* Generator = Profile->Generator.IsValid() ? Profile->Generator.Get() : Profile->Generator.LoadSynchronous();
	UYILootTable* LootTable = nullptr;
	if (!Generator)
	{
		LootTable = Profile->LootTable.IsValid() ? Profile->LootTable.Get() : Profile->LootTable.LoadSynchronous();
	}

	const int32 MinRolls = FMath::Max(0, Profile->MinRolls);
	const int32 MaxRolls = FMath::Max(MinRolls, Profile->MaxRolls);
	const int32 RollCount = (MaxRolls > MinRolls) ? RNG.RandRange(MinRolls, MaxRolls) : MinRolls;

	for (int32 RollIndex = 0; RollIndex < RollCount; ++RollIndex)
	{
		const int32 RollSeed = RNG.RandRange(1, TNumericLimits<int32>::Max());

		FYIBagItem Item;
		bool bProduced = false;

		if (Generator)
		{
			FGameplayTag RarityTag;
			int32 NumPrefixes = 0;
			int32 NumSuffixes = 0;
			bProduced = Generator->GenerateItem(Level, RollSeed, Item, RarityTag, NumPrefixes, NumSuffixes);
		}
		else if (LootTable)
		{
			TSoftObjectPtr<UYIItemDefinition> RolledDef;
			int32 RolledCount = 1;
			if (LootTable->RollDefinition(Level, RollSeed, RolledDef, RolledCount))
			{
				UYIItemDefinition* Def = RolledDef.IsValid() ? RolledDef.Get() : RolledDef.LoadSynchronous();
				if (Def)
				{
					Item.Item.Definition = Def;
					Item.Item.Count = FMath::Max(1, RolledCount);
					Item.Size = Def->GetEffectiveDefaultSize();
					Item.Pos = FIntPoint::ZeroValue;
					bProduced = true;
				}
			}
		}

		if (!bProduced)
		{
			continue;
		}

		if (RouteItem(Item))
		{
			++Result.NumGeneratedRolls;
		}
	}

	Result.bSuccess = (Result.NumGeneratedRolls + Result.NumGuaranteedDrops) > 0;
	if (Result.bSuccess)
	{
		if (Profile->bOneShot)
		{
			bHasDropped = true;
		}
		Result.Message = FText::Format(
			NSLOCTEXT("YOLOInventory", "LootDrop_Result_Ok", "Drop generated. Rolls: {0}, Guaranteed: {1}, Pickups: {2}, Inventory: {3}"),
			FText::AsNumber(Result.NumGeneratedRolls),
			FText::AsNumber(Result.NumGuaranteedDrops),
			FText::AsNumber(Result.NumSpawnedPickups),
			FText::AsNumber(Result.NumAddedToInventory));
	}
	else
	{
		Result.Message = NSLOCTEXT("YOLOInventory", "LootDrop_Result_Empty", "Drop resolved but produced no delivered items.");
	}

	return Result;
}
