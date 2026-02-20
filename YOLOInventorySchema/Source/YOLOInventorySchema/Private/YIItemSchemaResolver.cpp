#include "YIItemSchemaResolver.h"

#include "YIItemDefinition.h"
#include "YIItemFragments.h"
#include "YIItemTraitAsset.h"

namespace
{
	FCriticalSection GSnapshotCacheCS;
	TMap<FObjectKey, TSharedPtr<FYIItemSchemaSnapshot>> GSnapshotCache;

	const FInstancedStruct* YI_FindFragmentByStruct(const TArray<FInstancedStruct>& Fragments, const UScriptStruct* FragmentStruct)
	{
		if (!FragmentStruct)
		{
			return nullptr;
		}

		for (const FInstancedStruct& Fragment : Fragments)
		{
			if (Fragment.GetScriptStruct() == FragmentStruct)
			{
				return &Fragment;
			}
		}
		return nullptr;
	}

	const FInstancedStruct* YI_FindResolvedFragment(
		const UYIItemDefinition* Definition,
		const UScriptStruct* FragmentStruct,
		TSet<const UYIItemDefinition*>& VisitedDefinitions)
	{
		if (!Definition || !FragmentStruct || VisitedDefinitions.Contains(Definition))
		{
			return nullptr;
		}
		VisitedDefinitions.Add(Definition);

		if (const FInstancedStruct* Local = YI_FindFragmentByStruct(Definition->DefinitionFragments, FragmentStruct))
		{
			return Local;
		}

		for (int32 TraitIndex = Definition->Traits.Num() - 1; TraitIndex >= 0; --TraitIndex)
		{
			const UYIItemTraitAsset* Trait = Definition->Traits[TraitIndex].LoadSynchronous();
			if (!Trait)
			{
				continue;
			}

			if (const FInstancedStruct* TraitFragment = YI_FindFragmentByStruct(Trait->DefinitionFragments, FragmentStruct))
			{
				return TraitFragment;
			}
		}

		const UYIItemDefinition* Parent = Definition->ParentDefinition.LoadSynchronous();
		return YI_FindResolvedFragment(Parent, FragmentStruct, VisitedDefinitions);
	}

	template<typename TFragmentType>
	const TFragmentType* YI_GetResolvedFragment(const UYIItemDefinition* Definition)
	{
		TSet<const UYIItemDefinition*> Visited;
		if (const FInstancedStruct* Fragment = YI_FindResolvedFragment(Definition, TFragmentType::StaticStruct(), Visited))
		{
			return Fragment->GetPtr<TFragmentType>();
		}
		return nullptr;
	}

	void YI_CollectMergedTags(const UYIItemDefinition* Definition, TSet<const UYIItemDefinition*>& VisitedDefinitions, FGameplayTagContainer& OutTags)
	{
		if (!Definition || VisitedDefinitions.Contains(Definition))
		{
			return;
		}
		VisitedDefinitions.Add(Definition);

		if (const UYIItemDefinition* Parent = Definition->ParentDefinition.LoadSynchronous())
		{
			YI_CollectMergedTags(Parent, VisitedDefinitions, OutTags);
		}

		for (const TSoftObjectPtr<UYIItemTraitAsset>& TraitPtr : Definition->Traits)
		{
			const UYIItemTraitAsset* Trait = TraitPtr.LoadSynchronous();
			if (!Trait)
			{
				continue;
			}

			for (const FInstancedStruct& Fragment : Trait->DefinitionFragments)
			{
				if (const FYIItemClassificationDefinitionFragment* Classification = Fragment.GetPtr<FYIItemClassificationDefinitionFragment>())
				{
					OutTags.AppendTags(Classification->Tags);
					break;
				}
			}
		}

		for (const FInstancedStruct& Fragment : Definition->DefinitionFragments)
		{
			if (const FYIItemClassificationDefinitionFragment* Classification = Fragment.GetPtr<FYIItemClassificationDefinitionFragment>())
			{
				OutTags.AppendTags(Classification->Tags);
				break;
			}
		}
	}

	void YI_BuildSnapshot(const UYIItemDefinition* Definition, FYIItemSchemaSnapshot& OutSnapshot)
	{
		OutSnapshot = FYIItemSchemaSnapshot();
		if (!Definition)
		{
			return;
		}

		OutSnapshot.UniqueCode = Definition->UniqueCode;
		OutSnapshot.TemplateId = Definition->TemplateId;

		OutSnapshot.Display.DisplayName = FText::FromString(Definition->GetName());

		if (const FYIItemUIDefinitionFragment* UI = YI_GetResolvedFragment<FYIItemUIDefinitionFragment>(Definition))
		{
			if (!UI->DisplayName.IsEmpty())
			{
				OutSnapshot.Display.DisplayName = UI->DisplayName;
			}
			if (!UI->Description.IsEmpty())
			{
				OutSnapshot.Display.Description = UI->Description;
			}
			if (UI->Icon.ToSoftObjectPath().IsValid())
			{
				OutSnapshot.Display.Icon = UI->Icon;
			}
		}

		if (const FYIItemClassificationDefinitionFragment* Classification = YI_GetResolvedFragment<FYIItemClassificationDefinitionFragment>(Definition))
		{
			OutSnapshot.Classification.ItemType = Classification->ItemType;
			OutSnapshot.Classification.RarityTag = Classification->RarityTag;
		}

		{
			TSet<const UYIItemDefinition*> VisitedTags;
			YI_CollectMergedTags(Definition, VisitedTags, OutSnapshot.Classification.Tags);
		}

		if (const FYIItemAudioDefinitionFragment* Audio = YI_GetResolvedFragment<FYIItemAudioDefinitionFragment>(Definition))
		{
			OutSnapshot.Audio.AudioTag = Audio->AudioTag;
			OutSnapshot.Audio.SoundProfileOverride = Audio->SoundProfileOverride.ToSoftObjectPath();
		}

		if (const FYIItemLayoutDefinitionFragment* Layout = YI_GetResolvedFragment<FYIItemLayoutDefinitionFragment>(Definition))
		{
			OutSnapshot.Layout.DefaultSize = FIntPoint(FMath::Max(1, Layout->DefaultSize.X), FMath::Max(1, Layout->DefaultSize.Y));
			OutSnapshot.Layout.bAllowRotation = Layout->bAllowRotation;
		}

		if (const FYIItemStackingDefinitionFragment* Stacking = YI_GetResolvedFragment<FYIItemStackingDefinitionFragment>(Definition))
		{
			OutSnapshot.Stacking.bAllowStacking = Stacking->bAllowStacking;
			OutSnapshot.Stacking.MaxStackCount = FMath::Max(1, Stacking->MaxStackCount);
			OutSnapshot.Stacking.bUseRiskChecks = Stacking->bUseRiskChecks;
		}

		if (const FYIItemAffixDefinitionFragment* Affix = YI_GetResolvedFragment<FYIItemAffixDefinitionFragment>(Definition))
		{
			OutSnapshot.Affix.MinRandomModifiers = Affix->MinRandomModifiers;
			OutSnapshot.Affix.MaxRandomModifiers = Affix->MaxRandomModifiers;
			OutSnapshot.Affix.PrefixPool = Affix->PrefixPool.ToSoftObjectPath();
			OutSnapshot.Affix.SuffixPool = Affix->SuffixPool.ToSoftObjectPath();

			OutSnapshot.Affix.TemplateAffixes.Reserve(Affix->TemplateAffixes.Num());
			for (const TSoftObjectPtr<UYIAffixAsset>& TemplateAffix : Affix->TemplateAffixes)
			{
				const FSoftObjectPath AffixPath = TemplateAffix.ToSoftObjectPath();
				if (AffixPath.IsValid())
				{
					OutSnapshot.Affix.TemplateAffixes.Add(AffixPath);
				}
			}
		}

		if (const FYIItemEquipmentDefinitionFragment* Equipment = YI_GetResolvedFragment<FYIItemEquipmentDefinitionFragment>(Definition))
		{
			OutSnapshot.Equipment.PrimaryEquipSlot = Equipment->PrimaryEquipSlot;
			OutSnapshot.Equipment.OccupiedSlots = Equipment->OccupiedSlots;
		}

		if (const FYIItemRulesDefinitionFragment* Rules = YI_GetResolvedFragment<FYIItemRulesDefinitionFragment>(Definition))
		{
			OutSnapshot.Rules.bUniquePerType = Rules->bUniquePerType;
			OutSnapshot.Rules.EquipSlotCost = FMath::Max(1, Rules->EquipSlotCost);
		}

		if (const FYIItemContainerDefinitionFragment* Container = YI_GetResolvedFragment<FYIItemContainerDefinitionFragment>(Definition))
		{
			OutSnapshot.Container.bIsContainerItem = Container->bIsContainerItem;
			OutSnapshot.Container.ContainerTemplateBag = Container->ContainerTemplateBag.ToSoftObjectPath();
			OutSnapshot.Container.ContainerDefaultGridSize =
				FIntPoint(FMath::Max(1, Container->ContainerDefaultGridSize.X), FMath::Max(1, Container->ContainerDefaultGridSize.Y));
		}

		if (const FYIItemAttributeModsDefinitionFragment* AttributeMods = YI_GetResolvedFragment<FYIItemAttributeModsDefinitionFragment>(Definition))
		{
			OutSnapshot.AttributeMods.AttributeMods.Reserve(AttributeMods->AttributeMods.Num());
			for (const TSoftObjectPtr<UYIAttributeModAsset>& Mod : AttributeMods->AttributeMods)
			{
				const FSoftObjectPath ModPath = Mod.ToSoftObjectPath();
				if (ModPath.IsValid())
				{
					OutSnapshot.AttributeMods.AttributeMods.Add(ModPath);
				}
			}
		}
	}
}

const FYIItemSchemaSnapshot& YIItemSchema::ResolveSnapshot(const UYIItemDefinition* Definition)
{
	static const FYIItemSchemaSnapshot EmptySnapshot;
	if (!Definition)
	{
		return EmptySnapshot;
	}

	const FObjectKey DefinitionKey(Definition);
	{
		FScopeLock CacheLock(&GSnapshotCacheCS);
		if (const TSharedPtr<FYIItemSchemaSnapshot>* Existing = GSnapshotCache.Find(DefinitionKey))
		{
			if (Existing->IsValid())
			{
				return *Existing->Get();
			}
		}
	}

	FYIItemSchemaSnapshot BuiltSnapshot;
	YI_BuildSnapshot(Definition, BuiltSnapshot);

	FScopeLock CacheLock(&GSnapshotCacheCS);
	TSharedPtr<FYIItemSchemaSnapshot>& Cached = GSnapshotCache.FindOrAdd(DefinitionKey);
	if (!Cached.IsValid())
	{
		Cached = MakeShared<FYIItemSchemaSnapshot>();
	}
	*Cached = MoveTemp(BuiltSnapshot);
	return *Cached.Get();
}

void YIItemSchema::InvalidateSnapshotCache(const UYIItemDefinition* Definition)
{
	if (!Definition)
	{
		return;
	}

	FScopeLock CacheLock(&GSnapshotCacheCS);
	GSnapshotCache.Remove(FObjectKey(Definition));
}

void YIItemSchema::InvalidateAllSnapshotCaches()
{
	FScopeLock CacheLock(&GSnapshotCacheCS);
	GSnapshotCache.Reset();
}

const FInstancedStruct* YIItemSchema::FindResolvedDefinitionFragmentByStruct(const UYIItemDefinition* Definition, const UScriptStruct* FragmentStruct)
{
	TSet<const UYIItemDefinition*> Visited;
	return YI_FindResolvedFragment(Definition, FragmentStruct, Visited);
}

