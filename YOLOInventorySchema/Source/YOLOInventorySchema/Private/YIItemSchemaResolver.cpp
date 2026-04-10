#include "YIItemSchemaResolver.h"

#include "Async/Async.h"
#include "HAL/Event.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ScopeRWLock.h"
#include "UObject/ObjectKey.h"
#include "YIItemDefinition.h"
#include "YIItemFragments.h"
#include "YIItemTraitAsset.h"

void FYIItemSchemaSnapshot::RebuildResolvedFragmentIndex()
{
	ResolvedFragmentIndexByStruct.Reset();
	ResolvedCustomFragmentIndexByTag.Reset();
	for (int32 Index = 0; Index < ResolvedDefinitionFragments.Num(); ++Index)
	{
		const FInstancedStruct& Fragment = ResolvedDefinitionFragments[Index];
		const UScriptStruct* FragmentStruct = Fragment.GetScriptStruct();
		if (FragmentStruct)
		{
			ResolvedFragmentIndexByStruct.Add(FragmentStruct, Index);
		}

		if (const FYIItemCustomDefinitionFragment* CustomFragment = Fragment.GetPtr<FYIItemCustomDefinitionFragment>())
		{
			if (CustomFragment->FragmentTag.IsValid())
			{
				ResolvedCustomFragmentIndexByTag.Add(CustomFragment->FragmentTag, Index);
			}
		}
	}
}

const FInstancedStruct* FYIItemSchemaSnapshot::FindResolvedFragmentByStruct(const UScriptStruct* FragmentStruct) const
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	if (const int32* Index = ResolvedFragmentIndexByStruct.Find(FragmentStruct))
	{
		return ResolvedDefinitionFragments.IsValidIndex(*Index) ? &ResolvedDefinitionFragments[*Index] : nullptr;
	}
	return nullptr;
}

const FYIItemCustomDefinitionFragment* FYIItemSchemaSnapshot::FindResolvedCustomFragmentByTag(const FGameplayTag& FragmentTag) const
{
	if (!FragmentTag.IsValid())
	{
		return nullptr;
	}

	TArray<int32> Indices;
	ResolvedCustomFragmentIndexByTag.MultiFind(FragmentTag, Indices);
	if (Indices.Num() > 0)
	{
		if (ResolvedDefinitionFragments.IsValidIndex(Indices[0]))
		{
			return ResolvedDefinitionFragments[Indices[0]].GetPtr<FYIItemCustomDefinitionFragment>();
		}
	}
	return nullptr;
}

void FYIItemSchemaSnapshot::FindResolvedCustomFragmentsByTag(const FGameplayTag& FragmentTag, TArray<const FYIItemCustomDefinitionFragment*>& OutFragments) const
{
	OutFragments.Reset();
	if (!FragmentTag.IsValid())
	{
		return;
	}

	TArray<int32> Indices;
	ResolvedCustomFragmentIndexByTag.MultiFind(FragmentTag, Indices);
	OutFragments.Reserve(Indices.Num());
	for (const int32 Index : Indices)
	{
		if (!ResolvedDefinitionFragments.IsValidIndex(Index))
		{
			continue;
		}

		if (const FYIItemCustomDefinitionFragment* Custom = ResolvedDefinitionFragments[Index].GetPtr<FYIItemCustomDefinitionFragment>())
		{
			OutFragments.Add(Custom);
		}
	}
}

namespace
{
	FRWLock GSnapshotCacheLock;
	TMap<FObjectKey, YIItemSchema::FYIItemSchemaSnapshotHandle> GSnapshotCache;

	thread_local YIItemSchema::FYIItemSchemaSnapshotHandle GThreadLocalSnapshotHandle;
	thread_local YIItemSchema::FYIItemSchemaSnapshotHandle GThreadLocalFragmentHandle;

	YIItemSchema::FYIItemSchemaSnapshotHandle YI_GetCachedSnapshotHandle(const FObjectKey& DefinitionKey)
	{
		FReadScopeLock ReadLock(GSnapshotCacheLock);
		if (const YIItemSchema::FYIItemSchemaSnapshotHandle* Existing = GSnapshotCache.Find(DefinitionKey))
		{
			return *Existing;
		}
		return nullptr;
	}

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

	const UYIItemDefinition* YI_GetLoadedParent(const UYIItemDefinition* Definition)
	{
		return Definition ? Definition->ParentDefinition.Get() : nullptr;
	}

	void YI_MergeFragmentArray(
		const TArray<FInstancedStruct>& SourceFragments,
		TArray<FInstancedStruct>& InOutResolvedFragments,
		TMap<const UScriptStruct*, int32>& InOutIndexByStruct)
	{
		for (const FInstancedStruct& Fragment : SourceFragments)
		{
			const UScriptStruct* FragmentStruct = Fragment.GetScriptStruct();
			if (!FragmentStruct)
			{
				continue;
			}

			const bool bTreatAsUnique = YIIsDefinitionFragmentUnique(Fragment);

			if (bTreatAsUnique)
			{
				if (const int32* ExistingIndex = InOutIndexByStruct.Find(FragmentStruct))
				{
					InOutResolvedFragments[*ExistingIndex] = Fragment;
				}
				else
				{
					const int32 NewIndex = InOutResolvedFragments.Add(Fragment);
					InOutIndexByStruct.Add(FragmentStruct, NewIndex);
				}
				continue;
			}

			InOutResolvedFragments.Add(Fragment);
		}
	}

	void YI_CollectResolvedFragmentsRecursive(
		const UYIItemDefinition* Definition,
		TSet<const UYIItemDefinition*>& VisitedDefinitions,
		TArray<FInstancedStruct>& InOutResolvedFragments,
		TMap<const UScriptStruct*, int32>& InOutIndexByStruct)
	{
		if (!Definition || VisitedDefinitions.Contains(Definition))
		{
			return;
		}
		VisitedDefinitions.Add(Definition);

		YI_CollectResolvedFragmentsRecursive(
			YI_GetLoadedParent(Definition),
			VisitedDefinitions,
			InOutResolvedFragments,
			InOutIndexByStruct);

		for (const TSoftObjectPtr<UYIItemTraitAsset>& TraitPtr : Definition->Traits)
		{
			const UYIItemTraitAsset* Trait = TraitPtr.Get();
			if (!Trait)
			{
				continue;
			}

			YI_MergeFragmentArray(Trait->DefinitionFragments, InOutResolvedFragments, InOutIndexByStruct);
		}

		YI_MergeFragmentArray(Definition->DefinitionFragments, InOutResolvedFragments, InOutIndexByStruct);
	}

	void YI_CollectMergedClassificationTagsRecursive(
		const UYIItemDefinition* Definition,
		TSet<const UYIItemDefinition*>& VisitedDefinitions,
		FGameplayTagContainer& OutTags)
	{
		if (!Definition || VisitedDefinitions.Contains(Definition))
		{
			return;
		}
		VisitedDefinitions.Add(Definition);

		YI_CollectMergedClassificationTagsRecursive(YI_GetLoadedParent(Definition), VisitedDefinitions, OutTags);

		for (const TSoftObjectPtr<UYIItemTraitAsset>& TraitPtr : Definition->Traits)
		{
			const UYIItemTraitAsset* Trait = TraitPtr.Get();
			if (!Trait)
			{
				continue;
			}

			if (const FInstancedStruct* Fragment = YI_FindFragmentByStruct(Trait->DefinitionFragments, FYIItemClassificationDefinitionFragment::StaticStruct()))
			{
				if (const FYIItemClassificationDefinitionFragment* Classification = Fragment->GetPtr<FYIItemClassificationDefinitionFragment>())
				{
					OutTags.AppendTags(Classification->Tags);
				}
			}
		}

		if (const FInstancedStruct* Fragment = YI_FindFragmentByStruct(Definition->DefinitionFragments, FYIItemClassificationDefinitionFragment::StaticStruct()))
		{
			if (const FYIItemClassificationDefinitionFragment* Classification = Fragment->GetPtr<FYIItemClassificationDefinitionFragment>())
			{
				OutTags.AppendTags(Classification->Tags);
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

		TSet<const UYIItemDefinition*> VisitedDefinitions;
		TMap<const UScriptStruct*, int32> ResolvedIndexByStruct;
		YI_CollectResolvedFragmentsRecursive(
			Definition,
			VisitedDefinitions,
			OutSnapshot.ResolvedDefinitionFragments,
			ResolvedIndexByStruct);
		OutSnapshot.RebuildResolvedFragmentIndex();

		if (const FYIItemUIDefinitionFragment* UI = OutSnapshot.FindResolvedFragment<FYIItemUIDefinitionFragment>())
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

		if (const FYIItemClassificationDefinitionFragment* Classification = OutSnapshot.FindResolvedFragment<FYIItemClassificationDefinitionFragment>())
		{
			OutSnapshot.Classification.ItemType = Classification->ItemType;
			OutSnapshot.Classification.RarityTag = Classification->RarityTag;
		}

		{
			TSet<const UYIItemDefinition*> VisitedTags;
			YI_CollectMergedClassificationTagsRecursive(Definition, VisitedTags, OutSnapshot.Classification.Tags);
		}

		if (const FYIItemAudioDefinitionFragment* Audio = OutSnapshot.FindResolvedFragment<FYIItemAudioDefinitionFragment>())
		{
			OutSnapshot.Audio.AudioTag = Audio->AudioTag;
			OutSnapshot.Audio.SoundProfileOverride = Audio->SoundProfileOverride.ToSoftObjectPath();
		}

		if (const FYIItemLayoutDefinitionFragment* Layout = OutSnapshot.FindResolvedFragment<FYIItemLayoutDefinitionFragment>())
		{
			OutSnapshot.Layout.DefaultSize = FIntPoint(FMath::Max(1, Layout->DefaultSize.X), FMath::Max(1, Layout->DefaultSize.Y));
			OutSnapshot.Layout.bAllowRotation = Layout->bAllowRotation;
		}

		if (const FYIItemStackingDefinitionFragment* Stacking = OutSnapshot.FindResolvedFragment<FYIItemStackingDefinitionFragment>())
		{
			OutSnapshot.Stacking.MaxStackCount = FMath::Max(1, Stacking->MaxStackCount);
			OutSnapshot.Stacking.bUseRiskChecks = Stacking->bUseRiskChecks;
		}

		if (const FYIItemAffixDefinitionFragment* Affix = OutSnapshot.FindResolvedFragment<FYIItemAffixDefinitionFragment>())
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

		if (const FYIItemEquipmentDefinitionFragment* Equipment = OutSnapshot.FindResolvedFragment<FYIItemEquipmentDefinitionFragment>())
		{
			OutSnapshot.Equipment.PrimaryEquipSlot = Equipment->PrimaryEquipSlot;
			OutSnapshot.Equipment.OccupiedSlots = Equipment->OccupiedSlots;
		}

		if (const FYIItemRulesDefinitionFragment* Rules = OutSnapshot.FindResolvedFragment<FYIItemRulesDefinitionFragment>())
		{
			OutSnapshot.Rules.bUniquePerType = Rules->bUniquePerType;
			OutSnapshot.Rules.EquipSlotCost = FMath::Max(1, Rules->EquipSlotCost);
		}

		if (const FYIItemContainerDefinitionFragment* Container = OutSnapshot.FindResolvedFragment<FYIItemContainerDefinitionFragment>())
		{
			OutSnapshot.Container.bIsContainerItem = Container->bIsContainerItem;
			OutSnapshot.Container.ContainerTemplateBag = Container->ContainerTemplateBag.ToSoftObjectPath();
			OutSnapshot.Container.ContainerDefaultGridSize =
				FIntPoint(FMath::Max(1, Container->ContainerDefaultGridSize.X), FMath::Max(1, Container->ContainerDefaultGridSize.Y));
		}

		if (const FYIItemAttributeModsDefinitionFragment* AttributeMods = OutSnapshot.FindResolvedFragment<FYIItemAttributeModsDefinitionFragment>())
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

	YIItemSchema::FYIItemSchemaSnapshotHandle YI_BuildAndCacheSnapshotHandle_GameThread(const UYIItemDefinition* Definition)
	{
		check(IsInGameThread());
		if (!Definition)
		{
			return nullptr;
		}

		const FObjectKey DefinitionKey(Definition);
		if (YIItemSchema::FYIItemSchemaSnapshotHandle Existing = YI_GetCachedSnapshotHandle(DefinitionKey))
		{
			return Existing;
		}

		FYIItemSchemaSnapshot BuiltSnapshot;
		YI_BuildSnapshot(Definition, BuiltSnapshot);
		YIItemSchema::FYIItemSchemaSnapshotHandle NewHandle = MakeShared<FYIItemSchemaSnapshot, ESPMode::ThreadSafe>(MoveTemp(BuiltSnapshot));

		FWriteScopeLock WriteLock(GSnapshotCacheLock);
		YIItemSchema::FYIItemSchemaSnapshotHandle& Cached = GSnapshotCache.FindOrAdd(DefinitionKey);
		if (!Cached.IsValid())
		{
			Cached = NewHandle;
		}
		return Cached;
	}

	YIItemSchema::FYIItemSchemaSnapshotHandle YI_ResolveSnapshotHandle_ThreadSafe(const UYIItemDefinition* Definition)
	{
		static const YIItemSchema::FYIItemSchemaSnapshotHandle EmptyHandle = MakeShared<FYIItemSchemaSnapshot, ESPMode::ThreadSafe>();
		if (!Definition)
		{
			return EmptyHandle;
		}

		const FObjectKey DefinitionKey(Definition);
		if (YIItemSchema::FYIItemSchemaSnapshotHandle Existing = YI_GetCachedSnapshotHandle(DefinitionKey))
		{
			return Existing;
		}

		if (IsInGameThread())
		{
			return YI_BuildAndCacheSnapshotHandle_GameThread(Definition);
		}

		// UObject graph traversal for snapshot build must run on the game thread.
		FEvent* BuildEvent = FPlatformProcess::GetSynchEventFromPool(true);
		YIItemSchema::FYIItemSchemaSnapshotHandle BuiltHandle = EmptyHandle;
		AsyncTask(ENamedThreads::GameThread, [&BuiltHandle, BuildEvent, Definition]()
		{
			BuiltHandle = YI_BuildAndCacheSnapshotHandle_GameThread(Definition);
			BuildEvent->Trigger();
		});
		BuildEvent->Wait();
		FPlatformProcess::ReturnSynchEventToPool(BuildEvent);
		return BuiltHandle.IsValid() ? BuiltHandle : EmptyHandle;
	}
}

YIItemSchema::FYIItemSchemaSnapshotHandle YIItemSchema::ResolveSnapshotHandle(const UYIItemDefinition* Definition)
{
	return YI_ResolveSnapshotHandle_ThreadSafe(Definition);
}

const FYIItemSchemaSnapshot& YIItemSchema::ResolveSnapshot(const UYIItemDefinition* Definition)
{
	static const FYIItemSchemaSnapshot EmptySnapshot;
	GThreadLocalSnapshotHandle = ResolveSnapshotHandle(Definition);
	return GThreadLocalSnapshotHandle.IsValid() ? *GThreadLocalSnapshotHandle : EmptySnapshot;
}

void YIItemSchema::InvalidateSnapshotCache(const UYIItemDefinition* Definition)
{
	if (!Definition)
	{
		return;
	}

	FWriteScopeLock WriteLock(GSnapshotCacheLock);
	GSnapshotCache.Remove(FObjectKey(Definition));
}

void YIItemSchema::InvalidateAllSnapshotCaches()
{
	FWriteScopeLock WriteLock(GSnapshotCacheLock);
	GSnapshotCache.Reset();
}

void YIItemSchema::WarmupDefinition(const UYIItemDefinition* Definition)
{
	ResolveSnapshotHandle(Definition);
}

const FInstancedStruct* YIItemSchema::FindResolvedDefinitionFragmentByStruct(const UYIItemDefinition* Definition, const UScriptStruct* FragmentStruct)
{
	GThreadLocalFragmentHandle = ResolveSnapshotHandle(Definition);
	if (!GThreadLocalFragmentHandle.IsValid())
	{
		return nullptr;
	}
	return GThreadLocalFragmentHandle->FindResolvedFragmentByStruct(FragmentStruct);
}

const FYIItemCustomDefinitionFragment* YIItemSchema::FindCustomDefinitionFragment(const UYIItemDefinition* Definition, const FGameplayTag& FragmentTag)
{
	GThreadLocalFragmentHandle = ResolveSnapshotHandle(Definition);
	if (!GThreadLocalFragmentHandle.IsValid())
	{
		return nullptr;
	}
	return GThreadLocalFragmentHandle->FindResolvedCustomFragmentByTag(FragmentTag);
}

void YIItemSchema::FindCustomDefinitionFragments(const UYIItemDefinition* Definition, const FGameplayTag& FragmentTag, TArray<const FYIItemCustomDefinitionFragment*>& OutFragments)
{
	OutFragments.Reset();
	GThreadLocalFragmentHandle = ResolveSnapshotHandle(Definition);
	if (!GThreadLocalFragmentHandle.IsValid())
	{
		return;
	}
	GThreadLocalFragmentHandle->FindResolvedCustomFragmentsByTag(FragmentTag, OutFragments);
}
