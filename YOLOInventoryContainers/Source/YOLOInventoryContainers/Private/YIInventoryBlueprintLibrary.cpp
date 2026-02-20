#include "YIInventoryBlueprintLibrary.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIAffixAsset.h"
#include "YIAffixPoolAsset.h"
#include "YIItemInstance.h"
#include "StructUtils/InstancedStruct.h"
#include "GameplayTagContainer.h"
#include "YIInventoryTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "YOLOInventorySettings.h"
#include "YIRequirement.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagsManager.h"
#include "GameFramework/PlayerState.h"
#include "YIItemRegistrySubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "YIItemInstance.h"

static UYIItemDefinition* YI_FindDefinitionByCode(int64 Code);
static FYIItemInstance YI_MakeItemInstanceByCode(int64 Code, int32 Count);

namespace
{
	static void YI_GetEffectiveAffixDefinitionData(
		const UYIItemDefinition* Definition,
		TArray<TSoftObjectPtr<UYIAffixAsset>>& OutTemplateAffixes,
		int32& OutMinRandomModifiers,
		int32& OutMaxRandomModifiers,
		TSoftObjectPtr<UYIAffixPoolAsset>& OutPrefixPool,
		TSoftObjectPtr<UYIAffixPoolAsset>& OutSuffixPool)
	{
		OutTemplateAffixes.Reset();
		OutMinRandomModifiers = 0;
		OutMaxRandomModifiers = 0;
		OutPrefixPool = nullptr;
		OutSuffixPool = nullptr;

		if (!Definition)
		{
			return;
		}

		Definition->GetEffectiveAffixDefinition(
			OutTemplateAffixes,
			OutMinRandomModifiers,
			OutMaxRandomModifiers,
			OutPrefixPool,
			OutSuffixPool);
	}
}

bool UYIInventoryBlueprintLibrary::BuildAffixSnapshot(const UYIAffixAsset* Affix, int32 Level, int32 Seed, bool bRollValue, FYIAffixInstance& OutSnapshot)
{
	OutSnapshot = FYIAffixInstance();
	if (!Affix)
	{
		return false;
	}

	FYIAffixResolvedDefinitionData Effective;
	Affix->GetEffectiveDefinitionData(Effective);

	OutSnapshot.Source = const_cast<UYIAffixAsset*>(Affix);
	OutSnapshot.SourceCode = Affix->UniqueCode;
	OutSnapshot.SourceTemplateId = Affix->TemplateId;
	OutSnapshot.TierRolled = FMath::Max(1, Effective.Tier);
	OutSnapshot.Seed = Seed;
	OutSnapshot.ConflictGroupCache = Effective.ConflictGroup;
	OutSnapshot.DisplayNameCache = Effective.DisplayName;
	OutSnapshot.KindCache = Effective.Kind;

	float Rolled = Effective.MinValue;
	if (bRollValue)
	{
		if (!FMath::IsNearlyEqual(Effective.MinValue, Effective.MaxValue))
		{
			FRandomStream Stream(Seed);
			Rolled = Stream.FRandRange(Effective.MinValue, Effective.MaxValue);
		}
		if (Effective.ValueByLevel.GetRichCurveConst())
		{
			Rolled *= Effective.ValueByLevel.GetRichCurveConst()->Eval(Level, 1.f);
		}
		Rolled *= FMath::Max(1, Effective.PowerLevel);
	}

	OutSnapshot.RolledValue = Rolled;
	return true;
}

bool UYIInventoryBlueprintLibrary::ValidateAffixSnapshot(const FYIAffixInstance& Snapshot, int32 Level, float Tolerance)
{
	UYIAffixAsset* Affix = Snapshot.Source.IsValid() ? Snapshot.Source.Get() : Snapshot.Source.LoadSynchronous();
	if (!Affix)
	{
		return false;
	}
	if (Snapshot.SourceCode != 0 && Affix->UniqueCode != Snapshot.SourceCode)
	{
		return false;
	}

	FYIAffixResolvedDefinitionData Effective;
	Affix->GetEffectiveDefinitionData(Effective);

	if (!Snapshot.ConflictGroupCache.IsNone() && Snapshot.ConflictGroupCache != Effective.ConflictGroup)
	{
		return false;
	}
	if (Snapshot.TierRolled < 1)
	{
		return false;
	}

	if (Snapshot.Seed != 0)
	{
		FYIAffixInstance Expected;
		if (!BuildAffixSnapshot(Affix, Level, Snapshot.Seed, true, Expected))
		{
			return false;
		}
		if (!FMath::IsNearlyEqual(Expected.RolledValue, Snapshot.RolledValue, Tolerance))
		{
			return false;
		}
	}

	return true;
}

bool UYIInventoryBlueprintLibrary::ApplyAffixSnapshot(FYIBagItem& Item, const FYIAffixInstance& Snapshot, bool bValidateAgainstSource, int32 Level)
{
	UYIAffixAsset* Affix = Snapshot.Source.IsValid() ? Snapshot.Source.Get() : Snapshot.Source.LoadSynchronous();
	if (!Affix)
	{
		return false;
	}

	// Validate AllowedItemTags against the item's definition tags.
	UYIItemDefinition* Def = Item.Item.Definition.IsValid() ? Item.Item.Definition.Get() : Item.Item.Definition.LoadSynchronous();
	if (!Def)
	{
		return false;
	}

	FYIAffixResolvedDefinitionData Effective;
	Affix->GetEffectiveDefinitionData(Effective);

	if (!Effective.AllowedItemTags.IsEmpty() && !Def->Tags.HasAny(Effective.AllowedItemTags))
	{
		UE_LOG(LogTemp, Verbose, TEXT("ApplyAffixSnapshot: affix '%s' not allowed on item '%s' by tags"), *Affix->GetName(), *Def->GetName());
		return false;
	}

	if (!Snapshot.ConflictGroupCache.IsNone())
	{
		for (const FYIAffixInstance& Existing : Item.Item.Affixes)
		{
			if (Existing.ConflictGroupCache == Snapshot.ConflictGroupCache)
			{
				UE_LOG(LogTemp, Verbose, TEXT("ApplyAffixSnapshot: affix '%s' conflicts with group '%s'"), *Affix->GetName(), *Snapshot.ConflictGroupCache.ToString());
				return false;
			}
		}
	}

	if (bValidateAgainstSource && !ValidateAffixSnapshot(Snapshot, Level))
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyAffixSnapshot: validation failed for affix '%s'."), *Affix->GetName());
		return false;
	}

	Item.Item.Affixes.Add(Snapshot);
	Item.Item.SyncLegacyToCoreFragments();
	UYIInventoryBlueprintLibrary::UpdateCustomStackKey(Item.Item);
	return true;
}

bool UYIInventoryBlueprintLibrary::AddRolledAffix(FYIBagItem& Item, UYIAffixAsset* Affix, int32 Level, int32 Seed, float& OutRolledValue)
{
	OutRolledValue = 0.f;
	if (!Affix)
	{
		return false;
	}
	FYIAffixInstance Snapshot;
	if (!BuildAffixSnapshot(Affix, Level, Seed, true, Snapshot))
	{
		return false;
	}
	if (!ApplyAffixSnapshot(Item, Snapshot, true, Level))
	{
		return false;
	}
	OutRolledValue = Snapshot.RolledValue;
	return true;
}

int32 UYIInventoryBlueprintLibrary::ApplyTemplateAffixesToInstance(const UYIItemDefinition* Definition, FYIItemInstance& Instance)
{
	if (!Definition)
	{
		return 0;
	}

	TArray<TSoftObjectPtr<UYIAffixAsset>> EffectiveTemplateAffixes;
	int32 EffectiveMin = 0;
	int32 EffectiveMax = 0;
	TSoftObjectPtr<UYIAffixPoolAsset> EffectivePrefixPool;
	TSoftObjectPtr<UYIAffixPoolAsset> EffectiveSuffixPool;
	YI_GetEffectiveAffixDefinitionData(Definition, EffectiveTemplateAffixes, EffectiveMin, EffectiveMax, EffectivePrefixPool, EffectiveSuffixPool);

	int32 Added = 0;
	for (const TSoftObjectPtr<UYIAffixAsset>& TmplSoft : EffectiveTemplateAffixes)
	{
		UYIAffixAsset* A = TmplSoft.IsValid() ? TmplSoft.Get() : TmplSoft.LoadSynchronous();
		if (A)
		{
			FYIAffixResolvedDefinitionData EffectiveAffixData;
			A->GetEffectiveDefinitionData(EffectiveAffixData);

			// Validate allowed tags on definition
			if (!EffectiveAffixData.AllowedItemTags.IsEmpty() && !Definition->Tags.HasAny(EffectiveAffixData.AllowedItemTags))
			{
				UE_LOG(LogTemp, Verbose, TEXT("ApplyTemplateAffixesToInstance: skipping affix '%s' due to AllowedItemTags"), *A->GetName());
				continue;
			}
			// Skip if conflict group already present
			if (!EffectiveAffixData.ConflictGroup.IsNone())
			{
				bool bFound = false;
				for (const FYIAffixInstance& Existing : Instance.Affixes)
				{
					if (Existing.ConflictGroupCache == EffectiveAffixData.ConflictGroup) { bFound = true; break; }
				}
				if (bFound) { UE_LOG(LogTemp, Verbose, TEXT("ApplyTemplateAffixesToInstance: skipping affix '%s' due to ConflictGroup"), *A->GetName()); continue; }
			}

			FYIAffixInstance Snapshot;
			if (!BuildAffixSnapshot(A, 1, 0, false, Snapshot))
			{
				continue;
			}
			Instance.Affixes.Add(Snapshot);
			++Added;
		}
	}
	Instance.SyncLegacyToCoreFragments();
	// Recompute custom stack key after applying templates
	UYIInventoryBlueprintLibrary::UpdateCustomStackKey(Instance);
	return Added;
}

bool UYIInventoryBlueprintLibrary::HasEvolutionCapability(const UYIItemDefinition* Definition)
{
	static const FGameplayTag EvolutionTag = FGameplayTag::RequestGameplayTag(FName("Capability.Evolution"), false);
	(void)Definition;
	return false; // capabilities removed
}

bool UYIInventoryBlueprintLibrary::GenerateAffixesForInstance(FYIBagItem& Item, int32 Level, int32 Seed, int32 NumPrefixes, int32 NumSuffixes)
{
	// If caller didn't specify counts, use definition's Min/MaxRandomModifiers to derive totals
	UYIItemDefinition* DefForCounts = Item.Item.Definition.IsValid() ? Item.Item.Definition.Get() : Item.Item.Definition.LoadSynchronous();
	TArray<TSoftObjectPtr<UYIAffixAsset>> EffectiveTemplateAffixes;
	int32 EffectiveMinMods = 0;
	int32 EffectiveMaxMods = 0;
	TSoftObjectPtr<UYIAffixPoolAsset> EffectivePrefixPool;
	TSoftObjectPtr<UYIAffixPoolAsset> EffectiveSuffixPool;
	YI_GetEffectiveAffixDefinitionData(DefForCounts, EffectiveTemplateAffixes, EffectiveMinMods, EffectiveMaxMods, EffectivePrefixPool, EffectiveSuffixPool);

	if (DefForCounts && NumPrefixes <= 0 && NumSuffixes <= 0)
	{
		FRandomStream CStream(Seed ^ 0x5F3759DF);
		const int32 MinMods = FMath::Max(0, EffectiveMinMods);
		const int32 MaxMods = FMath::Max(MinMods, EffectiveMaxMods);
		const int32 TotalMods = (MaxMods > MinMods) ? CStream.RandRange(MinMods, MaxMods) : MinMods;
		// Split approximately half/half between prefix/suffix, bias prefix when odd
		NumPrefixes = TotalMods / 2 + (TotalMods & 1);
		NumSuffixes = TotalMods - NumPrefixes;
	}
	UYIItemDefinition* Def = Item.Item.Definition.IsValid() ? Item.Item.Definition.Get() : Item.Item.Definition.LoadSynchronous();
	if (!Def) return false;

	FRandomStream Stream(Seed);
	int32 Added = 0;

	// Helper lambda to sample N affixes from a pool
	auto SampleFromPool = [&](TSoftObjectPtr<UYIAffixPoolAsset> PoolSoft, int32 Count) -> int32
	{
		if (Count <= 0) return 0;
		UYIAffixPoolAsset* Pool = PoolSoft.IsValid() ? PoolSoft.Get() : PoolSoft.LoadSynchronous();
		if (!Pool) return 0;
		int32 Got = 0;
		int32 Attempts = 0;
		int32 MaxAttempts = FMath::Max(8, Count * 6);
		while (Got < Count && Attempts < MaxAttempts)
		{
			UYIAffixAsset* A = Pool->SampleAffix(Stream, Level);
			if (!A) { ++Attempts; continue; }
			float Rolled = 0.f;
			int32 SeedForAff = Stream.RandRange(1, INT32_MAX);
			if (AddRolledAffix(Item, A, Level, SeedForAff, Rolled)) { ++Got; ++Added; }
			++Attempts;
		}
		return Got;
	};

	SampleFromPool(EffectivePrefixPool, NumPrefixes);
	SampleFromPool(EffectiveSuffixPool, NumSuffixes);

	return Added > 0;
}

bool UYIInventoryBlueprintLibrary::GetEvolutionXP(const FYIBagItem& Item, int32& OutXP)
{
	// Legacy evolution state removed. Return 0 and false to indicate no state.
	OutXP = 0;
	(void)Item;
	return false;
}

void UYIInventoryBlueprintLibrary::SetEvolutionXP(FYIBagItem& Item, int32 XP)
{
	// No-op placeholder for evolution state. Intentionally left blank for future capability-backed storage.
	(void)Item; (void)XP;
}

int32 UYIInventoryBlueprintLibrary::GetBuyPrice(const UYIItemDefinition* Definition, float PriceMultiplier)
{
	(void)Definition;
	const float Mult = FMath::Max(0.f, PriceMultiplier);
	return FMath::RoundToInt(0 * Mult); // price TBD on definition
}

int32 UYIInventoryBlueprintLibrary::GetSellPrice(const UYIItemDefinition* Definition, float PriceMultiplier)
{
	(void)Definition;
	const float Mult = FMath::Max(0.f, PriceMultiplier);
	return FMath::RoundToInt(0 * Mult); // price TBD on definition
}

bool UYIInventoryBlueprintLibrary::TransferItemBetweenBags(UYIInventoryBag* Source, UYIInventoryBag* Dest, int32 Index, int32 Count, int32& OutDestIndex)
{
	OutDestIndex = INDEX_NONE;
	if (!Source || !Dest || Source == Dest || !Source->Items.IsValidIndex(Index))
	{
		return false;
	}
	FYIBagItem Src = Source->Items[Index];
	UYIItemDefinition* Def = Src.Item.Definition.IsValid() ? Src.Item.Definition.Get() : Src.Item.Definition.LoadSynchronous();
	if (!Def)
	{
		return false;
	}

	if (!Dest->CanAcceptItemDefinition(Def))
	{
		return false;
	}

	FYIBagItem ToPlace = Src;
	const bool bStacking = Def->IsRuntimeStackingAllowed();
	if (bStacking && Count > 0)
	{
		ToPlace.Item.Count = FMath::Clamp(Count, 1, Src.Item.Count);
	}

	// Try combine into existing stack
	int32 Existing = INDEX_NONE;
	if (Src.Item.CustomStackKey != 0)
	{
		Existing = Dest->FindExistingStackIndexForItem(Src);
	}
	if (Existing == INDEX_NONE)
	{
		Existing = Dest->FindExistingStackIndex(Def);
	}

	if (Existing != INDEX_NONE && bStacking)
	{
		FYIBagItem& DestIt = Dest->Items[Existing];
		int32 Room = Def->GetEffectiveMaxStackCount() - DestIt.Item.Count;
		if (Room > 0)
		{
			int32 MoveCount = FMath::Min(Room, ToPlace.Item.Count);
			DestIt.Item.Count += MoveCount;
			Source->Items[Index].Item.Count -= MoveCount;
			if (Source->Items[Index].Item.Count <= 0)
			{
				Source->RemoveItem(Index);
			}
			else
			{
				Source->MarkPackageDirty();
				Source->OnChanged.Broadcast();
			}
			OutDestIndex = Existing;
			Dest->MarkPackageDirty();
			Dest->OnChanged.Broadcast();
			// Broadcast transfer event on both bags
			Source->OnItemTransferred.Broadcast(Source, Dest, Index, Existing);
			Dest->OnItemTransferred.Broadcast(Source, Dest, Index, Existing);
			return true;
		}
	}

	// Find placement
	FIntPoint Pos;
	if (!Dest->FindFirstFit(ToPlace.Size, Pos))
	{
		return false;
	}
	ToPlace.Pos = Pos;

	// Add to destination first so failed adds never lose source data.
	const bool bSavedAutoMerge = Dest->bAutoMergeOnAdd;
	Dest->bAutoMergeOnAdd = false;
	OutDestIndex = Dest->AddBagItem(ToPlace);
	Dest->bAutoMergeOnAdd = bSavedAutoMerge;
	if (OutDestIndex == INDEX_NONE)
	{
		return false;
	}

	// Reduce source count only after destination insert succeeds.
	if (bStacking && Count > 0)
	{
		Source->Items[Index].Item.Count -= ToPlace.Item.Count;
		if (Source->Items[Index].Item.Count <= 0)
		{
			if (!Source->RemoveItem(Index))
			{
				Dest->RemoveItem(OutDestIndex);
				OutDestIndex = INDEX_NONE;
				return false;
			}
		}
		else
		{
			Source->MarkPackageDirty();
			Source->OnChanged.Broadcast();
		}
	}
	else
	{
		// Move whole stack
		if (!Source->RemoveItem(Index))
		{
			Dest->RemoveItem(OutDestIndex);
			OutDestIndex = INDEX_NONE;
			return false;
		}
	}

	Source->OnItemTransferred.Broadcast(Source, Dest, Index, OutDestIndex);
	Dest->OnItemTransferred.Broadcast(Source, Dest, Index, OutDestIndex);
	return true;
}

bool UYIInventoryBlueprintLibrary::GetFirstEmptyPosForItem(const UYIInventoryBag* Bag, const UYIItemDefinition* Definition, FIntPoint& OutPos)
{
	if (!Bag || !Definition) return false;
	return Bag->FindFirstFit(Definition->GetEffectiveDefaultSize(), OutPos);
}

bool UYIInventoryBlueprintLibrary::AddItemToBagByCode(UYIInventoryBag* Bag, int64 Code, int32 Count)
{
	if (!Bag || Code == 0 || Count <= 0)
	{
		return false;
	}

	UYIItemDefinition* Def = YI_FindDefinitionByCode(Code);
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItemToBagByCode: definition for code %lld not found"), (long long)Code);
		return false;
	}

	FYIBagItem NewItem;
	NewItem.Item = YI_MakeItemInstanceByCode(Code, Count);
	NewItem.Size = Def->GetEffectiveDefaultSize();
	int32 NewIdx = Bag->AddBagItem(NewItem);
	return NewIdx != INDEX_NONE;
}

bool UYIInventoryBlueprintLibrary::GetItemTooltipData(const UYIInventoryBag* Bag, int32 Index, FYITooltipData& OutData, const FYIRequirementContext& RequirementContext)
{
	OutData = FYITooltipData();
	if (!Bag || !Bag->Items.IsValidIndex(Index)) return false;
	const FYIBagItem& It = Bag->Items[Index];
	UYIItemDefinition* Def = It.Item.Definition.IsValid()? It.Item.Definition.Get() : It.Item.Definition.LoadSynchronous();
	if (!Def) return false;

	const TArray<FYIAffixInstance>* RuntimeAffixes = &It.Item.Affixes;
	if (const FYIItemAffixesFragment* AffixFragment = It.Item.GetAffixesFragment())
	{
		RuntimeAffixes = &AffixFragment->Values;
	}

	const TMap<FName, float>* RuntimeAttributes = &It.Item.Attributes;
	if (const FYIItemAttributesFragment* AttrFragment = It.Item.GetAttributesFragment())
	{
		RuntimeAttributes = &AttrFragment->Values;
	}

	FText EffectiveDisplayName;
	FText EffectiveDescription;
	TSoftObjectPtr<UTexture2D> EffectiveIcon;
	Def->GetEffectiveDisplayData(EffectiveDisplayName, EffectiveDescription, EffectiveIcon);

	OutData.NameBase = EffectiveDisplayName;
	OutData.Title = EffectiveDisplayName;
	// Simple prefix/suffix extraction from affixes (best-effort)
	if (RuntimeAffixes->Num() > 0)
	{
		OutData.NamePrefix = (*RuntimeAffixes)[0].DisplayNameCache;
		if (RuntimeAffixes->Num() > 1)
		{
			OutData.NameSuffix = RuntimeAffixes->Last().DisplayNameCache;
		}
	}
	// Build combined name
	FString FullNameStr;
	if (!OutData.NamePrefix.IsEmpty()) { FullNameStr += OutData.NamePrefix.ToString(); FullNameStr += TEXT(" "); }
	if (!OutData.NameBase.IsEmpty()) { FullNameStr += OutData.NameBase.ToString(); }
	if (!OutData.NameSuffix.IsEmpty())
	{
		if (!FullNameStr.IsEmpty()) { FullNameStr += TEXT(" "); }
		FullNameStr += OutData.NameSuffix.ToString();
	}
	if (!FullNameStr.IsEmpty())
	{
		OutData.FullName = FText::FromString(FullNameStr);
		OutData.Title = OutData.FullName;
	}
	// Derive rarity color from definition using designer-authored rarity tags.
	OutData.RarityColor = UYIInventoryBlueprintLibrary::GetColorForRarityTag(Def->RarityTag);
	// Peek at a UI stack entry if present for description and icon
	// We do not keep a runtime merged stack list here; just use asset fields if available
	OutData.Description = EffectiveDescription;
	OutData.Icon = EffectiveIcon;
	// Affix preview lines
	OutData.AffixLines.Reset();
	// 1) Template affixes from definition
	TArray<TSoftObjectPtr<UYIAffixAsset>> EffectiveTemplateAffixes;
	int32 EffectiveMinMods = 0;
	int32 EffectiveMaxMods = 0;
	TSoftObjectPtr<UYIAffixPoolAsset> EffectivePrefixPool;
	TSoftObjectPtr<UYIAffixPoolAsset> EffectiveSuffixPool;
	YI_GetEffectiveAffixDefinitionData(Def, EffectiveTemplateAffixes, EffectiveMinMods, EffectiveMaxMods, EffectivePrefixPool, EffectiveSuffixPool);

	for (const TSoftObjectPtr<UYIAffixAsset>& TmplSoft : EffectiveTemplateAffixes)
	{
		UYIAffixAsset* Src = TmplSoft.IsValid() ? TmplSoft.Get() : TmplSoft.LoadSynchronous();
		if (Src)
		{
			FYIAffixResolvedDefinitionData AffixData;
			Src->GetEffectiveDefinitionData(AffixData);
			FText Line = !AffixData.TooltipFormat.IsEmpty() ? AffixData.TooltipFormat : AffixData.DisplayName;
			OutData.AffixLines.Add(Line);
		}
	}
	// 2) Rolled affixes on instance
	for (const FYIAffixInstance& A : *RuntimeAffixes)
	{
		UYIAffixAsset* Src = A.Source.IsValid() ? A.Source.Get() : A.Source.LoadSynchronous();
		if (Src)
		{
			FYIAffixResolvedDefinitionData AffixData;
			Src->GetEffectiveDefinitionData(AffixData);
			// Format using TooltipFormat if provided; fallback to DisplayName
			FText Line;
			if (!AffixData.TooltipFormat.IsEmpty())
			{
				// One-arg replacement: {0}
				FFormatNamedArguments Args; Args.Add(TEXT("0"), FText::AsNumber(A.RolledValue));
				Line = FText::Format(AffixData.TooltipFormat, Args);
			}
			else if (!A.DisplayNameCache.IsEmpty())
			{
				Line = A.DisplayNameCache;
			}
			else
			{
				Line = AffixData.DisplayName;
			}
			OutData.AffixLines.Add(Line);
		}
	}

	// Requirements: deferred for future implementation; keep defaults
	OutData.bAllRequirementsMet = true;

	// Attributes (raw key/value from instance attributes map)
	for (const TPair<FName,float>& Pair : *RuntimeAttributes)
	{
		FYITooltipAttributeLine Line; Line.Label = FText::FromName(Pair.Key); Line.Value = Pair.Value;
		OutData.AttributeLines.Add(Line);
	}

	// Durability heuristics: look for keys in attributes
	auto TryFindAttr = [&RuntimeAttributes](const FName& Key, float& OutVal)->bool
	{
		if (const float* V = RuntimeAttributes->Find(Key)) { OutVal = *V; return true; }
		return false;
	};
	float Cur=0.f, Max=0.f;
	bool bHasCur = TryFindAttr(TEXT("Durability"), Cur) || TryFindAttr(TEXT("CurrentDurability"), Cur);
	bool bHasMax = TryFindAttr(TEXT("DurabilityMax"), Max) || TryFindAttr(TEXT("MaxDurability"), Max);
	if (bHasCur || bHasMax)
	{
		OutData.bHasDurability = true;
		OutData.CurrentDurability = Cur;
		OutData.MaxDurability = bHasMax ? Max : Cur;
	}

	// Economy: sell price (stub uses blueprint library getter)
	OutData.SellPrice = UYIInventoryBlueprintLibrary::GetSellPrice(Def, 1.0f);
	return true;
}

bool UYIInventoryBlueprintLibrary::GetItemTooltipData(const UYIInventoryBag* Bag, int32 Index, FYITooltipData& OutData)
{
	static const FYIRequirementContext EmptyCtx;
	return GetItemTooltipData(Bag, Index, OutData, EmptyCtx);
}

FLinearColor UYIInventoryBlueprintLibrary::GetColorForRarityTag(const FGameplayTag& RarityTag)
{
	// First, consult plugin settings palette (editable in Project Settings -> YOLO Inventory)
	const UYOLOInventorySettings& Settings = UYOLOInventorySettings::Get();
	for (const FYIRarityColorEntry& Entry : Settings.RarityColors)
	{
		if (!Entry.RarityTag.IsValid())
		{
			continue;
		}
		if (RarityTag.IsValid() && RarityTag.MatchesTag(Entry.RarityTag))
		{
			return Entry.Color;
		}
	}
	// Fallback: try to map by common name tokens (designer tags like Rarity.Common, Rarity.Epic, etc.)
	if (RarityTag.IsValid())
	{
		FString Name = RarityTag.GetTagName().ToString();
		if (Name.Contains(TEXT("Unique"))) return FLinearColor(0.95f, 0.55f, 0.15f, 1.f);
		if (Name.Contains(TEXT("Common"))) return YI_GetRarityColor(EYOLOItemRarity::Common);
		if (Name.Contains(TEXT("Uncommon"))) return YI_GetRarityColor(EYOLOItemRarity::Uncommon);
		if (Name.Contains(TEXT("Rare"))) return YI_GetRarityColor(EYOLOItemRarity::Rare);
		if (Name.Contains(TEXT("Epic"))) return YI_GetRarityColor(EYOLOItemRarity::Epic);
		if (Name.Contains(TEXT("Legendary"))) return YI_GetRarityColor(EYOLOItemRarity::Legendary);
		if (Name.Contains(TEXT("Mythic"))) return YI_GetRarityColor(EYOLOItemRarity::Mythic);
	}
	return FLinearColor::White;
}

int64 UYIInventoryBlueprintLibrary::ComputeCustomStackKey(const FYIItemInstance& Instance)
{
	// Build canonical descriptor string: definition path + rotation + size + sorted affix descriptors
	FString Desc = Instance.Definition.ToSoftObjectPath().ToString();
	Desc += Instance.bRotated ? TEXT("|R1") : TEXT("|R0");
	// Derive size from definition (and rotation) for stack key
	FIntPoint Sz(1,1);
	if (UYIItemDefinition* Def = Instance.Definition.IsValid() ? Instance.Definition.Get() : Instance.Definition.LoadSynchronous())
	{
		Sz = Def->GetEffectiveDefaultSize();
		if (Instance.bRotated) Sz = FIntPoint(Sz.Y, Sz.X);
	}
	Desc += FString::Printf(TEXT("|S%d,%d"), Sz.X, Sz.Y);
	const TArray<FYIAffixInstance>* AffixSource = &Instance.Affixes;
	TArray<FYIAffixInstance> AffixScratch;
	if (const FYIItemAffixesFragment* AffixFragment = Instance.GetAffixesFragment())
	{
		AffixSource = &AffixFragment->Values;
	}
	else if (!Instance.Fragments.IsEmpty())
	{
		FYIItemInstance Copy = Instance;
		Copy.SyncCoreFragmentsToLegacy();
		AffixScratch = MoveTemp(Copy.Affixes);
		AffixSource = &AffixScratch;
	}

	TArray<FString> Parts;
	Parts.Reserve(AffixSource->Num());
	for (const FYIAffixInstance& A : *AffixSource)
	{
		const FString Src = (A.SourceCode != 0)
			? FString::Printf(TEXT("Code:%lld"), (long long)A.SourceCode)
			: A.Source.ToSoftObjectPath().ToString();
		const FString Val = FString::Printf(TEXT("%.4f"), A.RolledValue);
		Parts.Add(FString::Printf(TEXT("%s:%d:%s"), *Src, (int32)A.KindCache, *Val));
	}
	Parts.Sort();
	for (const FString& P : Parts) Desc += TEXT("|") + P;

	const TMap<FName, float>* AttrSource = &Instance.Attributes;
	TMap<FName, float> AttrScratch;
	if (const FYIItemAttributesFragment* AttrFragment = Instance.GetAttributesFragment())
	{
		AttrSource = &AttrFragment->Values;
	}
	else if (!Instance.Fragments.IsEmpty())
	{
		FYIItemInstance Copy = Instance;
		Copy.SyncCoreFragmentsToLegacy();
		AttrScratch = MoveTemp(Copy.Attributes);
		AttrSource = &AttrScratch;
	}

	TArray<FName> AttrKeys;
	AttrSource->GetKeys(AttrKeys);
	AttrKeys.Sort([](const FName& A, const FName& B)
	{
		return A.LexicalLess(B);
	});
	for (const FName& Key : AttrKeys)
	{
		if (const float* Val = AttrSource->Find(Key))
		{
			Desc += FString::Printf(TEXT("|A:%s=%.4f"), *Key.ToString(), *Val);
		}
	}

	if (const FYIItemDurabilityFragment* Dur = Instance.GetDurabilityFragment())
	{
		Desc += FString::Printf(TEXT("|D:%d:%.4f:%.4f"), Dur->bEnabled ? 1 : 0, Dur->Current, Dur->Max);
	}
	uint32 H = GetTypeHash(Desc);
	return static_cast<int64>(H);
}

void UYIInventoryBlueprintLibrary::UpdateCustomStackKey(FYIItemInstance& Instance)
{
	Instance.CustomStackKey = ComputeCustomStackKey(Instance);
}

void UYIInventoryBlueprintLibrary::InitializeFragmentsFromLegacy(FYIItemInstance& Instance)
{
	Instance.SyncLegacyToCoreFragments();
}

void UYIInventoryBlueprintLibrary::SyncLegacyFromFragments(FYIItemInstance& Instance)
{
	Instance.SyncCoreFragmentsToLegacy();
}

bool UYIInventoryBlueprintLibrary::GetItemDurability(const FYIItemInstance& Instance, float& OutCurrent, float& OutMax)
{
	OutCurrent = 0.f;
	OutMax = 0.f;

	if (const FYIItemDurabilityFragment* Dur = Instance.GetDurabilityFragment())
	{
		if (!Dur->bEnabled)
		{
			return false;
		}
		OutCurrent = Dur->Current;
		OutMax = Dur->Max;
		return true;
	}
	return false;
}

void UYIInventoryBlueprintLibrary::SetItemDurability(FYIItemInstance& Instance, float Current, float Max, bool bEnabled)
{
	if (FYIItemDurabilityFragment* Dur = Instance.GetMutableDurabilityFragment(true))
	{
		Dur->bEnabled = bEnabled;
		Dur->Current = FMath::Max(0.f, Current);
		Dur->Max = FMath::Max(0.f, Max);
	}

	// Legacy mirror for compatibility with existing tooltips/editor code paths.
	Instance.Attributes.Add(TEXT("Durability"), FMath::Max(0.f, Current));
	Instance.Attributes.Add(TEXT("DurabilityMax"), FMath::Max(0.f, Max));
	Instance.SyncLegacyToCoreFragments();
	UpdateCustomStackKey(Instance);
}

UYIItemDefinition* UYIInventoryBlueprintLibrary::FindItemDefinitionByTemplateId(const FString& TemplateId)
{
	if (TemplateId.IsEmpty()) return nullptr;
	FAssetRegistryModule& Arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter Filter;
	Filter.ClassPaths.Add(UYIItemDefinition::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> AssetDataList;
	Arm.Get().GetAssets(Filter, AssetDataList);
	for (const FAssetData& AD : AssetDataList)
	{
		// Fast path: check serialized tags (if present)
		FString TagVal;
		if (AD.GetTagValue(FName(TEXT("TemplateId")), TagVal) && TagVal == TemplateId)
		{
			UObject* Obj = AD.GetAsset();
			if (Obj) return Cast<UYIItemDefinition>(Obj);
			UObject* Loaded = AD.ToSoftObjectPath().TryLoad();
			return Cast<UYIItemDefinition>(Loaded);
		}
		// Fallback: load and inspect property
		UYIItemDefinition* Def = Cast<UYIItemDefinition>(AD.GetAsset());
		if (!Def) Def = Cast<UYIItemDefinition>(AD.ToSoftObjectPath().TryLoad());
		if (Def && Def->TemplateId == TemplateId) return Def;
	}
	return nullptr;
}

UYIAffixAsset* UYIInventoryBlueprintLibrary::FindAffixByTemplateId(const FString& TemplateId)
{
	if (TemplateId.IsEmpty()) return nullptr;
	FAssetRegistryModule& Arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter Filter;
	Filter.ClassPaths.Add(UYIAffixAsset::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> AssetDataList;
	Arm.Get().GetAssets(Filter, AssetDataList);
	for (const FAssetData& AD : AssetDataList)
	{
		FString TagVal;
		if (AD.GetTagValue(FName(TEXT("TemplateId")), TagVal) && TagVal == TemplateId)
		{
			UObject* Obj = AD.GetAsset();
			if (Obj) return Cast<UYIAffixAsset>(Obj);
			UObject* Loaded = AD.ToSoftObjectPath().TryLoad();
			return Cast<UYIAffixAsset>(Loaded);
		}
		UYIAffixAsset* A = Cast<UYIAffixAsset>(AD.GetAsset());
		if (!A) A = Cast<UYIAffixAsset>(AD.ToSoftObjectPath().TryLoad());
		if (A && A->TemplateId == TemplateId) return A;
	}
	return nullptr;
}

FYIItemInstance UYIInventoryBlueprintLibrary::MakeItemInstanceByTemplateId(const FString& TemplateId, int32 Count)
{
	UYIItemDefinition* Def = FindItemDefinitionByTemplateId(TemplateId);
	if (!Def) return FYIItemInstance();
	return YI_MakeItemInstanceByCode(Def->UniqueCode, Count);
}

static UYIItemDefinition* YI_FindDefinitionByCode(int64 Code)
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


bool UYIInventoryBlueprintLibrary::AddItemInstanceToBag(UYIInventoryBag* Bag, const FYIItemInstance& Instance)
{
	if (!Bag || Instance.Count <= 0 || !Instance.Definition.IsValid())
	{
		return false;
	}
	UYIItemDefinition* Def = Instance.Definition.Get();
	if (!Def)
	{
		Def = Instance.Definition.LoadSynchronous();
	}
	if (!Def)
	{
		return false;
	}
	FYIBagItem NewItem;
	NewItem.Item = Instance;
	NewItem.Item.SyncLegacyToCoreFragments();
	NewItem.Size = Def->GetEffectiveDefaultSize();
	int32 AddedIdx = Bag->AddBagItem(NewItem);
	return AddedIdx != INDEX_NONE;
}

static FYIItemInstance YI_MakeItemInstanceByCode(int64 Code, int32 Count)
{
	FYIItemInstance Out;
	Out.Count = Count;
	Out.Definition = YI_FindDefinitionByCode(Code);
	return Out;
}

const UYIItemSFXProfile* UYIInventoryBlueprintLibrary::ResolveItemSFXProfile(const UYIItemDefinition* Definition, const UYIItemSFXLibrary* Library)
{
	if (!Library)
	{
		return nullptr;
	}
	if (Definition && Definition->SoundProfileOverride)
	{
		return Definition->SoundProfileOverride;
	}

	FGameplayTag Tag;
	if (Definition)
	{
		Tag = Definition->AudioTag.IsValid() ? Definition->AudioTag : Definition->ItemType;
	}

	if (Tag.IsValid())
	{
		if (const TObjectPtr<UYIItemSFXProfile>* Found = Library->TagToProfile.Find(Tag))
		{
			return Found->Get();
		}
		const FGameplayTagContainer Parents = Tag.GetGameplayTagParents();
		for (const FGameplayTag& Parent : Parents)
		{
			if (const TObjectPtr<UYIItemSFXProfile>* FoundParent = Library->TagToProfile.Find(Parent))
			{
				return FoundParent->Get();
			}
		}
	}

	return Library->DefaultProfile;
}

USoundBase* UYIInventoryBlueprintLibrary::ResolveItemSFXSound(const UYIItemDefinition* Definition, const UYIItemSFXLibrary* Library, EYIItemSFXEvent Event)
{
	const UYIItemSFXProfile* Profile = ResolveItemSFXProfile(Definition, Library);
	if (!Profile)
	{
		return nullptr;
	}
	switch (Event)
	{
		case EYIItemSFXEvent::HoverItem: return Profile->HoverItemSound;
		case EYIItemSFXEvent::DragStart: return Profile->DragStartSound;
		case EYIItemSFXEvent::Drop: return Profile->DropSound;
		case EYIItemSFXEvent::Equip: return Profile->EquipSound;
		case EYIItemSFXEvent::Cancel: return Profile->CancelSound;
		case EYIItemSFXEvent::InvalidMove: return Profile->InvalidMoveSound;
		default: return nullptr;
	}
}

void UYIInventoryBlueprintLibrary::GetSuggestedInventoryGameplayTags(TArray<FGameplayTag>& OutTags)
{
	const UYOLOInventorySettings& Settings = UYOLOInventorySettings::Get();
	if (Settings.SuggestedInventoryTagPrefixes.Num() > 0)
	{
		GetSuggestedInventoryGameplayTagsByPrefixes(Settings.SuggestedInventoryTagPrefixes, OutTags, true);
	}
	else
	{
		static const TArray<FString> DefaultPrefixes = {
			TEXT("Inventory."),
			TEXT("Item."),
			TEXT("Equip."),
			TEXT("Bag."),
			TEXT("Actions."),
			TEXT("Affix."),
			TEXT("Loot."),
			TEXT("Craft."),
			TEXT("Rarity."),
			TEXT("Audio.")
		};
		GetSuggestedInventoryGameplayTagsByPrefixes(DefaultPrefixes, OutTags, true);
	}
}

void UYIInventoryBlueprintLibrary::GetSuggestedInventoryGameplayTagsByPrefixes(const TArray<FString>& Prefixes, TArray<FGameplayTag>& OutTags, bool bSortLexical)
{
	OutTags.Reset();

	FGameplayTagContainer AllTags;
	UGameplayTagsManager::Get().RequestAllGameplayTags(AllTags, true);

	TArray<FGameplayTag> AllTagArray;
	AllTags.GetGameplayTagArray(AllTagArray);
	AllTagArray.Reserve(AllTagArray.Num());

	for (const FGameplayTag& Tag : AllTagArray)
	{
		if (!Tag.IsValid())
		{
			continue;
		}

		const FString TagString = Tag.ToString();
		bool bMatch = Prefixes.Num() == 0;
		if (!bMatch)
		{
			for (const FString& Prefix : Prefixes)
			{
				if (Prefix.IsEmpty())
				{
					continue;
				}
				if (TagString.StartsWith(Prefix, ESearchCase::IgnoreCase))
				{
					bMatch = true;
					break;
				}
			}
		}

		if (bMatch)
		{
			OutTags.Add(Tag);
		}
	}

	if (bSortLexical)
	{
		OutTags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
		{
			return A.ToString() < B.ToString();
		});
	}
}
