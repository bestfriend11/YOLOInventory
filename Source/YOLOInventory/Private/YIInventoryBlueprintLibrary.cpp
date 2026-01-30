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
#include "YIItemBlueprintLibrary.h"
#include "Modules/ModuleManager.h"
#include "YOLOInventorySettings.h"
#include "YIRequirement.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "YIItemRegistrySubsystem.h"
#include "YITradeSessionActor.h"
#include "YIItemPickup.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "YIItemInstance.h"

static UYIItemDefinition* YI_FindDefinitionByCode(int64 Code);

bool UYIInventoryBlueprintLibrary::AddRolledAffix(FYIBagItem& Item, UYIAffixAsset* Affix, int32 Level, int32 Seed, float& OutRolledValue)
{
	OutRolledValue = 0.f;
	if (!Affix)
	{
		return false;
	}
	// Validate AllowedItemTags against the item's definition tags
	UYIItemDefinition* Def = Item.Item.Definition.IsValid() ? Item.Item.Definition.Get() : Item.Item.Definition.LoadSynchronous();
	if (!Def)
	{
		return false;
	}
	if (!Affix->AllowedItemTags.IsEmpty() && !Def->Tags.HasAny(Affix->AllowedItemTags))
	{
		// Affix not allowed on this item
		UE_LOG(LogTemp, Verbose, TEXT("AddRolledAffix: affix '%s' not allowed on item '%s' by tags"), *Affix->GetName(), *Def->GetName());
		return false;
	}
	// Check conflict group (avoid adding duplicates)
	if (!Affix->ConflictGroup.IsNone())
	{
		for (const FYIAffixInstance& Existing : Item.Item.Affixes)
		{
			if (Existing.ConflictGroupCache == Affix->ConflictGroup)
			{
				UE_LOG(LogTemp, Verbose, TEXT("AddRolledAffix: affix '%s' conflicts with existing affix group '%s'"), *Affix->GetName(), *Affix->ConflictGroup.ToString());
				return false;
			}
		}
	}

	FRandomStream Stream(Seed);
	float Rolled = Affix->MinValue;
	if (!FMath::IsNearlyEqual(Affix->MinValue, Affix->MaxValue))
	{
		Rolled = Stream.FRandRange(Affix->MinValue, Affix->MaxValue);
	}
	if (Affix->ValueByLevel.GetRichCurveConst())
	{
		Rolled *= Affix->ValueByLevel.GetRichCurveConst()->Eval(Level, 1.f);
	}
	// Apply affix power level as an additional multiplier
	Rolled *= FMath::Max(1, Affix->PowerLevel);
	FYIAffixInstance NewInst;
	NewInst.Source = Affix;
	NewInst.TierRolled = FMath::Max(1, Affix->Tier);
	NewInst.RolledValue = Rolled;
	NewInst.Seed = Seed;
	NewInst.ConflictGroupCache = Affix->ConflictGroup;
	NewInst.DisplayNameCache = Affix->DisplayName;
	Item.Item.Affixes.Add(NewInst);
	// Update stacking key to reflect new affix set
	UYIInventoryBlueprintLibrary::UpdateCustomStackKey(Item.Item);
	OutRolledValue = Rolled;
	return true;
}

int32 UYIInventoryBlueprintLibrary::ApplyTemplateAffixesToInstance(const UYIItemDefinition* Definition, FYIItemInstance& Instance)
{
	if (!Definition)
	{
		return 0;
	}
	int32 Added = 0;
	for (const TSoftObjectPtr<UYIAffixAsset>& TmplSoft : Definition->TemplateAffixes)
	{
		UYIAffixAsset* A = TmplSoft.IsValid() ? TmplSoft.Get() : TmplSoft.LoadSynchronous();
		if (A)
		{
			// Validate allowed tags on definition
			if (!A->AllowedItemTags.IsEmpty() && !Definition->Tags.HasAny(A->AllowedItemTags))
			{
				UE_LOG(LogTemp, Verbose, TEXT("ApplyTemplateAffixesToInstance: skipping affix '%s' due to AllowedItemTags"), *A->GetName());
				continue;
			}
			// Skip if conflict group already present
			if (!A->ConflictGroup.IsNone())
			{
				bool bFound = false;
				for (const FYIAffixInstance& Existing : Instance.Affixes)
				{
					if (Existing.ConflictGroupCache == A->ConflictGroup) { bFound = true; break; }
				}
				if (bFound) { UE_LOG(LogTemp, Verbose, TEXT("ApplyTemplateAffixesToInstance: skipping affix '%s' due to ConflictGroup"), *A->GetName()); continue; }
			}

			FYIAffixInstance NewInst;
			NewInst.Source = A;
			NewInst.TierRolled = FMath::Max(1, A->Tier);
			NewInst.RolledValue = 0.f; // templates carry fixed text/effects; no roll
			NewInst.Seed = 0;
			NewInst.ConflictGroupCache = A->ConflictGroup;
			NewInst.DisplayNameCache = A->DisplayName;
			Instance.Affixes.Add(NewInst);
			++Added;
		}
	}
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
	if (DefForCounts && NumPrefixes <= 0 && NumSuffixes <= 0)
	{
		FRandomStream CStream(Seed ^ 0x5F3759DF);
		const int32 MinMods = FMath::Max(0, DefForCounts->MinRandomModifiers);
		const int32 MaxMods = FMath::Max(MinMods, DefForCounts->MaxRandomModifiers);
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

	SampleFromPool(Def->PrefixPool, NumPrefixes);
	SampleFromPool(Def->SuffixPool, NumSuffixes);

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
	if (!Source || !Dest || !Source->Items.IsValidIndex(Index)) return false;
	FYIBagItem Src = Source->Items[Index];
	UYIItemDefinition* Def = Src.Item.Definition.IsValid()? Src.Item.Definition.Get() : Src.Item.Definition.LoadSynchronous();
	if (!Def) return false;

	FYIBagItem ToPlace = Src;
	if (Def->bAllowStacking && Def->MaxStackCount > 1 && Count > 0)
	{
		ToPlace.Item.Count = FMath::Clamp(Count, 1, Src.Item.Count);
	}

	// Try combine into existing stack
	int32 Existing = Dest->FindExistingStackIndex(Def);
	if (Existing != INDEX_NONE && Def->bAllowStacking && Def->MaxStackCount > 1)
	{
		FYIBagItem& DestIt = Dest->Items[Existing];
		int32 Room = Def->MaxStackCount - DestIt.Item.Count;
		if (Room > 0)
		{
			int32 MoveCount = FMath::Min(Room, ToPlace.Item.Count);
			DestIt.Item.Count += MoveCount;
			Source->Items[Index].Item.Count -= MoveCount;
			if (Source->Items[Index].Item.Count <= 0) { Source->RemoveItem(Index); }
			OutDestIndex = Existing;
			Dest->MarkPackageDirty();
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

	// Reduce source count and add to dest
	if (Def->bAllowStacking && Def->MaxStackCount > 1 && Count > 0)
	{
		Source->Items[Index].Item.Count -= ToPlace.Item.Count;
		if (Source->Items[Index].Item.Count <= 0) { Source->RemoveItem(Index); }
	}
	else
	{
		// Move whole stack
		Source->RemoveItem(Index);
	}
	OutDestIndex = Dest->AddBagItem(ToPlace);
	if (OutDestIndex != INDEX_NONE)
	{
		Source->OnItemTransferred.Broadcast(Source, Dest, Index, OutDestIndex);
		Dest->OnItemTransferred.Broadcast(Source, Dest, Index, OutDestIndex);
	}
	return OutDestIndex != INDEX_NONE;
}

bool UYIInventoryBlueprintLibrary::GetFirstEmptyPosForItem(const UYIInventoryBag* Bag, const UYIItemDefinition* Definition, FIntPoint& OutPos)
{
	if (!Bag || !Definition) return false;
	return Bag->FindFirstFit(Definition->DefaultSize, OutPos);
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
	NewItem.Item = UYIItemBlueprintLibrary::MakeItemInstanceByCode(Code, Count);
	NewItem.Size = Def->DefaultSize;
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
	OutData.NameBase = Def->DisplayName;
	OutData.Title = Def->DisplayName;
	// Simple prefix/suffix extraction from affixes (best-effort)
	if (It.Item.Affixes.Num() > 0)
	{
		OutData.NamePrefix = It.Item.Affixes[0].DisplayNameCache;
		if (It.Item.Affixes.Num() > 1)
		{
			OutData.NameSuffix = It.Item.Affixes.Last().DisplayNameCache;
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
	// Derive rarity color from definition (DS1-style tint via designer-defined rarity tag)
	OutData.RarityColor = UYIInventoryBlueprintLibrary::GetColorForRarityTag(Def->RarityTag);
	// Peek at a UI stack entry if present for description and icon
	// We do not keep a runtime merged stack list here; just use asset fields if available
	OutData.Description = Def->Description;
	OutData.Icon = Def->Icon;
	// Affix preview lines
	OutData.AffixLines.Reset();
	// 1) Template affixes from definition
	for (const TSoftObjectPtr<UYIAffixAsset>& TmplSoft : Def->TemplateAffixes)
	{
		UYIAffixAsset* Src = TmplSoft.IsValid() ? TmplSoft.Get() : TmplSoft.LoadSynchronous();
		if (Src)
		{
			FText Line = !Src->TooltipFormat.IsEmpty() ? Src->TooltipFormat : Src->DisplayName;
			OutData.AffixLines.Add(Line);
		}
	}
	// 2) Rolled affixes on instance
	for (const FYIAffixInstance& A : It.Item.Affixes)
	{
		UYIAffixAsset* Src = A.Source.IsValid() ? A.Source.Get() : A.Source.LoadSynchronous();
		if (Src)
		{
			// Format using TooltipFormat if provided; fallback to DisplayName
			FText Line;
			if (!Src->TooltipFormat.IsEmpty())
			{
				// One-arg replacement: {0}
				FFormatNamedArguments Args; Args.Add(TEXT("0"), FText::AsNumber(A.RolledValue));
				Line = FText::Format(Src->TooltipFormat, Args);
			}
			else if (!A.DisplayNameCache.IsEmpty())
			{
				Line = A.DisplayNameCache;
			}
			else
			{
				Line = Src->DisplayName;
			}
			OutData.AffixLines.Add(Line);
		}
	}

	// Requirements: deferred for future implementation; keep defaults
	OutData.bAllRequirementsMet = true;

	// Attributes (raw key/value from instance attributes map)
	for (const TPair<FName,float>& Pair : It.Item.Attributes)
	{
		FYITooltipAttributeLine Line; Line.Label = FText::FromName(Pair.Key); Line.Value = Pair.Value;
		OutData.AttributeLines.Add(Line);
	}

	// Durability heuristics: look for keys in attributes
	auto TryFindAttr = [&It](const FName& Key, float& OutVal)->bool
	{
		if (const float* V = It.Item.Attributes.Find(Key)) { OutVal = *V; return true; }
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
		Sz = Def->DefaultSize;
		if (Instance.bRotated) Sz = FIntPoint(Sz.Y, Sz.X);
	}
	Desc += FString::Printf(TEXT("|S%d,%d"), Sz.X, Sz.Y);
	TArray<FString> Parts;
	Parts.Reserve(Instance.Affixes.Num());
	for (const FYIAffixInstance& A : Instance.Affixes)
	{
		FString Src = A.Source.ToSoftObjectPath().ToString();
		FString Val = FString::Printf(TEXT("%.4f"), A.RolledValue);
		Parts.Add(Src + TEXT(":") + Val);
	}
	Parts.Sort();
	for (const FString& P : Parts) Desc += TEXT("|") + P;
	uint32 H = GetTypeHash(Desc);
	return static_cast<int64>(H);
}

void UYIInventoryBlueprintLibrary::UpdateCustomStackKey(FYIItemInstance& Instance)
{
	Instance.CustomStackKey = ComputeCustomStackKey(Instance);
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
	return UYIItemBlueprintLibrary::MakeItemInstanceByCode(Def->UniqueCode, Count);
}

static UWorld* YI_GetWorldFromContext(UObject* WorldContextObject)
{
	return WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
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

AYIItemPickup* UYIInventoryBlueprintLibrary::SpawnItemPickupByCode(UObject* WorldContextObject, int64 Code, const FTransform& Transform, int32 Count, TSubclassOf<AYIItemPickup> PickupClass)
{
	UWorld* World = YI_GetWorldFromContext(WorldContextObject);
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr; // enforce authority-only spawn
	}

	UYIItemDefinition* Definition = YI_FindDefinitionByCode(Code);
	if (!Definition)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnItemPickupByCode: definition for code %lld not found."), (long long)Code);
		return nullptr;
	}

	TSubclassOf<AYIItemPickup> ClassToSpawn = PickupClass ? PickupClass.Get() : AYIItemPickup::StaticClass();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = nullptr;
	Params.Instigator = nullptr;

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

AYIItemPickup* UYIInventoryBlueprintLibrary::SpawnItemPickup(UObject* WorldContextObject, UYIItemDefinition* Definition, const FTransform& Transform, int32 Count, TSubclassOf<AYIItemPickup> PickupClass)
{
	int64 Code = Definition ? Definition->UniqueCode : 0;
	return SpawnItemPickupByCode(WorldContextObject, Code, Transform, Count, PickupClass);
}

AYIItemPickup* UYIInventoryBlueprintLibrary::SpawnItemPickupFromInstance(UObject* WorldContextObject, const FYIItemInstance& Instance, const FTransform& Transform, TSubclassOf<AYIItemPickup> PickupClass)
{
	UWorld* World = YI_GetWorldFromContext(WorldContextObject);
	if (!World || World->GetNetMode() == NM_Client || Instance.Count <= 0)
	{
		return nullptr;
	}

	int64 Code = 0;
	if (Instance.Definition.IsValid())
	{
		Code = Instance.Definition.Get()->UniqueCode;
	}
	else if (Instance.Definition.ToSoftObjectPath().IsValid())
	{
		if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(Instance.Definition.LoadSynchronous()))
		{
			Code = Def->UniqueCode;
		}
	}

	TSubclassOf<AYIItemPickup> ClassToSpawn = PickupClass ? PickupClass.Get() : AYIItemPickup::StaticClass();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = nullptr;
	Params.Instigator = nullptr;

	AYIItemPickup* Pickup = World->SpawnActorDeferred<AYIItemPickup>(ClassToSpawn, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Pickup)
	{
		Pickup->ItemCode = Code;
		Pickup->Count = Instance.Count;
		// Convert to net-safe instance
		Pickup->ItemInstance.Definition = Instance.Definition;
		Pickup->ItemInstance.Count = Instance.Count;
		Pickup->ItemInstance.CustomStackKey = Instance.CustomStackKey;
		Pickup->ItemInstance.bRotated = Instance.bRotated;
		Pickup->ItemInstance.Affixes = Instance.Affixes;
		Pickup->ItemInstance.Attributes.Reset();
		for (const TPair<FName, float>& Pair : Instance.Attributes)
		{
			FYIAttributeKV KV; KV.Name = Pair.Key; KV.Value = Pair.Value;
			Pickup->ItemInstance.Attributes.Add(KV);
		}
		UGameplayStatics::FinishSpawningActor(Pickup, Transform);
		Pickup->RefreshVisuals(); // ensure visuals align
	}
	return Pickup;
}

bool UYIInventoryBlueprintLibrary::DropBagItemToWorld(UObject* WorldContextObject, UYIInventoryBag* Bag, int32 Index, const FTransform& SpawnTransform, int32 Count, TSubclassOf<AYIItemPickup> PickupClass)
{
	UWorld* World = YI_GetWorldFromContext(WorldContextObject);
	if (!World || World->GetNetMode() == NM_Client || !Bag || !Bag->Items.IsValidIndex(Index))
	{
		return false;
	}

	FYIBagItem& Src = Bag->Items[Index];
	int32 DropCount = (Count <= 0) ? Src.Item.Count : FMath::Min(Src.Item.Count, Count);
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
		FYIBagItem Removed = Src;
		Bag->Items.RemoveAt(Index);
		Bag->OnItemRemoved.Broadcast(Index, Removed);
	}
	Bag->OnChanged.Broadcast();
	return true;
}

bool UYIInventoryBlueprintLibrary::DestroyBagItem(UObject* WorldContextObject, UYIInventoryBag* Bag, int32 Index, int32 Count)
{
	if (!Bag || !Bag->Items.IsValidIndex(Index))
	{
		return false;
	}
	if (UWorld* World = YI_GetWorldFromContext(WorldContextObject))
	{
		if (World->GetNetMode() == NM_Client)
		{
			return false;
		}
	}

	FYIBagItem& Src = Bag->Items[Index];
	int32 DestroyCount = (Count <= 0) ? Src.Item.Count : FMath::Min(Src.Item.Count, Count);
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
		FYIBagItem Removed = Src;
		Bag->Items.RemoveAt(Index);
		Bag->OnItemRemoved.Broadcast(Index, Removed);
	}
	Bag->OnChanged.Broadcast();
	return true;
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
	NewItem.Size = Def->DefaultSize;
	int32 AddedIdx = Bag->AddBagItem(NewItem);
	return AddedIdx != INDEX_NONE;
}

bool UYIInventoryBlueprintLibrary::PickupItemActorIntoBag(UObject* WorldContextObject, UYIInventoryBag* Bag, AYIItemPickup* Pickup)
{
	UWorld* World = YI_GetWorldFromContext(WorldContextObject);
	if (!World || World->GetNetMode() == NM_Client || !Bag || !IsValid(Pickup))
	{
		return false;
	}

	const FYIItemInstanceNet& NetInstance = Pickup->ItemInstance;
	if (!NetInstance.Definition.IsValid() || NetInstance.Count <= 0)
	{
		return false;
	}

	// Convert back to full instance
	FYIItemInstance Full;
	Full.Definition = NetInstance.Definition;
	Full.Count = NetInstance.Count;
	Full.CustomStackKey = NetInstance.CustomStackKey;
	Full.bRotated = NetInstance.bRotated;
	Full.Affixes = NetInstance.Affixes;
	for (const FYIAttributeKV& KV : NetInstance.Attributes)
	{
		Full.Attributes.Add(KV.Name, KV.Value);
	}

	if (AddItemInstanceToBag(Bag, Full))
	{
		Pickup->Destroy();
		Bag->OnChanged.Broadcast();
		return true;
	}
	return false;
}

AYITradeSessionActor* UYIInventoryBlueprintLibrary::StartTradeSession(UObject* WorldContextObject, AActor* Initiator, AActor* Target, bool bTargetIsNPC)
{
	if (!WorldContextObject || !Initiator || !Target)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr; // server only
	}

	// Resolve initiating player controller/state
	APlayerController* PC = Cast<APlayerController>(Initiator->GetOwner());
	if (!PC)
	{
		if (APawn* Pawn = Cast<APawn>(Initiator))
		{
			PC = Cast<APlayerController>(Pawn->GetController());
		}
	}
	if (!PC || !PC->PlayerState)
	{
		return nullptr;
	}

	AYITradeSessionActor* Session = World->SpawnActor<AYITradeSessionActor>();
	if (!Session)
	{
		return nullptr;
	}

	Session->PlayerA = PC->PlayerState;
	Session->PawnA = Cast<APawn>(Initiator);

	if (bTargetIsNPC)
	{
		Session->bSideBIsNPC = true;
		Session->NPCPawn = Cast<APawn>(Target);
	}
	else
	{
		APawn* TargetPawn = Cast<APawn>(Target);
		APlayerController* TargetPC = TargetPawn ? Cast<APlayerController>(TargetPawn->GetController()) : nullptr;
		if (TargetPC && TargetPC->PlayerState)
		{
			Session->PlayerB = TargetPC->PlayerState;
			Session->PawnB = TargetPawn;
		}
	}

	Session->ForceNetUpdate();
	Session->RefreshInventoryViews(); // initial full inventory mirrors
	return Session;
}
