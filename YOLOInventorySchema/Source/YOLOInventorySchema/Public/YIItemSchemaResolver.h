#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "YIItemSchemaTypes.h"

class UYIAttributeModAsset;
class UYIItemDefinition;
class UYIItemSFXProfile;
class UScriptStruct;

namespace YIItemSchema
{
	/** Resolve (and cache) the effective schema snapshot for the given item definition. */
	YOLOINVENTORYSCHEMA_API const FYIItemSchemaSnapshot& ResolveSnapshot(const UYIItemDefinition* Definition);

	/** Invalidate cached snapshot for a specific definition (editor/runtime hot-reload safety). */
	YOLOINVENTORYSCHEMA_API void InvalidateSnapshotCache(const UYIItemDefinition* Definition);

	/** Invalidate all cached snapshots. */
	YOLOINVENTORYSCHEMA_API void InvalidateAllSnapshotCaches();

	/** Resolve a fragment by struct type using local -> trait -> parent precedence. */
	YOLOINVENTORYSCHEMA_API const FInstancedStruct* FindResolvedDefinitionFragmentByStruct(const UYIItemDefinition* Definition, const UScriptStruct* FragmentStruct);

	inline FText GetDisplayName(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Display.DisplayName;
	}

	inline FText GetDescription(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Display.Description;
	}

	inline TSoftObjectPtr<UTexture2D> GetIcon(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Display.Icon;
	}

	inline FGameplayTag GetItemType(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Classification.ItemType;
	}

	inline void GetTags(const UYIItemDefinition* Definition, FGameplayTagContainer& OutTags)
	{
		OutTags = ResolveSnapshot(Definition).Classification.Tags;
	}

	inline FGameplayTag GetRarityTag(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Classification.RarityTag;
	}

	inline FGameplayTag GetAudioTag(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Audio.AudioTag;
	}

	inline TSoftObjectPtr<UYIItemSFXProfile> GetSoundProfileOverride(const UYIItemDefinition* Definition)
	{
		return TSoftObjectPtr<UYIItemSFXProfile>(ResolveSnapshot(Definition).Audio.SoundProfileOverride);
	}

	inline FIntPoint GetDefaultSize(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Layout.DefaultSize;
	}

	inline bool IsRotationAllowed(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Layout.bAllowRotation;
	}

	inline bool IsStackingEnabled(const UYIItemDefinition* Definition)
	{
		const FYIItemSchemaStackingData& Stacking = ResolveSnapshot(Definition).Stacking;
		return Stacking.bAllowStacking && Stacking.MaxStackCount > 1;
	}

	inline int32 GetMaxStackCount(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Stacking.MaxStackCount;
	}

	inline bool IsUniquePerType(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Rules.bUniquePerType;
	}

	inline int32 GetEquipSlotCost(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Rules.EquipSlotCost;
	}

	inline FGameplayTag GetPrimaryEquipSlot(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Equipment.PrimaryEquipSlot;
	}

	inline void GetOccupiedEquipSlots(const UYIItemDefinition* Definition, FGameplayTagContainer& OutSlots)
	{
		OutSlots = ResolveSnapshot(Definition).Equipment.OccupiedSlots;
	}

	inline bool IsContainerItem(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Container.bIsContainerItem;
	}

	inline TSoftObjectPtr<UObject> GetContainerTemplateBag(const UYIItemDefinition* Definition)
	{
		return TSoftObjectPtr<UObject>(ResolveSnapshot(Definition).Container.ContainerTemplateBag);
	}

	inline FIntPoint GetContainerDefaultGridSize(const UYIItemDefinition* Definition)
	{
		return ResolveSnapshot(Definition).Container.ContainerDefaultGridSize;
	}

	inline void GetAttributeMods(const UYIItemDefinition* Definition, TArray<TSoftObjectPtr<UYIAttributeModAsset>>& OutMods)
	{
		OutMods.Reset();
		const TArray<FSoftObjectPath>& Paths = ResolveSnapshot(Definition).AttributeMods.AttributeMods;
		OutMods.Reserve(Paths.Num());
		for (const FSoftObjectPath& Path : Paths)
		{
			OutMods.Add(TSoftObjectPtr<UYIAttributeModAsset>(Path));
		}
	}
}

