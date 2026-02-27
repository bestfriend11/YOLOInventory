#include "YIItemDescriptionResolver.h"

#include "YIEquipmentFragments.h"
#include "YIInventoryBag.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YIItemDefinition.h"
#include "YIItemInstance.h"
#include "YIItemInstanceFragmentAccess.h"
#include "YIItemSchemaResolver.h"
#include "YIShopComponent.h"
#include "YITradeFragments.h"
#include "HAL/PlatformTime.h"
#include "Misc/ScopeRWLock.h"

namespace
{
	constexpr int32 GDescriptionCacheMaxEntries = 2048;

	struct FYIDescriptionCacheKey
	{
		FGuid ItemInstanceId;
		int32 BagRevision = 0;
		uint32 ContextHash = 0;

		bool operator==(const FYIDescriptionCacheKey& Other) const
		{
			return ItemInstanceId == Other.ItemInstanceId
				&& BagRevision == Other.BagRevision
				&& ContextHash == Other.ContextHash;
		}
	};

	uint32 GetTypeHash(const FYIDescriptionCacheKey& Key)
	{
		uint32 Hash = ::GetTypeHash(Key.ItemInstanceId.A);
		Hash = HashCombine(Hash, ::GetTypeHash(Key.ItemInstanceId.B));
		Hash = HashCombine(Hash, ::GetTypeHash(Key.ItemInstanceId.C));
		Hash = HashCombine(Hash, ::GetTypeHash(Key.ItemInstanceId.D));
		Hash = HashCombine(Hash, ::GetTypeHash(Key.BagRevision));
		Hash = HashCombine(Hash, Key.ContextHash);
		return Hash;
	}

	struct FYIDescriptionCacheValue
	{
		FText EconomyLine;
		int32 SellPrice = 0;
		bool bHasDurability = false;
		float CurrentDurability = 0.0f;
		float MaxDurability = 0.0f;
		TArray<FText> ExtraAffixLines;
		TArray<FYITooltipRequirementLine> ExtraRequirementLines;
		uint64 LastAccessCycle = 0;
	};

	FRWLock GDescriptionCacheLock;
	TMap<FYIDescriptionCacheKey, FYIDescriptionCacheValue> GDescriptionCache;

	void YI_TrimDescriptionCacheUnsafe()
	{
		if (GDescriptionCache.Num() <= GDescriptionCacheMaxEntries)
		{
			return;
		}

		int32 RemoveCount = GDescriptionCache.Num() - GDescriptionCacheMaxEntries;
		while (RemoveCount-- > 0 && GDescriptionCache.Num() > 0)
		{
			uint64 OldestCycle = TNumericLimits<uint64>::Max();
			FYIDescriptionCacheKey OldestKey;
			bool bHasOldest = false;
			for (const TPair<FYIDescriptionCacheKey, FYIDescriptionCacheValue>& Pair : GDescriptionCache)
			{
				if (Pair.Value.LastAccessCycle < OldestCycle)
				{
					OldestCycle = Pair.Value.LastAccessCycle;
					OldestKey = Pair.Key;
					bHasOldest = true;
				}
			}

			if (!bHasOldest)
			{
				break;
			}
			GDescriptionCache.Remove(OldestKey);
		}
	}

	uint32 BuildContextHash(const FYIItemDescriptionContext& Context)
	{
		uint32 Hash = ::GetTypeHash(Context.Shop);
		Hash = HashCombine(Hash, ::GetTypeHash(Context.ViewerPlayerState));
		Hash = HashCombine(Hash, ::GetTypeHash(Context.bShopBuyContext));
		Hash = HashCombine(Hash, ::GetTypeHash(FMath::Max(1, Context.Count)));
		return Hash;
	}

	FYIDescriptionCacheKey BuildCacheKey(const FYIItemDescriptionContext& Context)
	{
		FYIDescriptionCacheKey Key;
		Key.ItemInstanceId = Context.Item ? Context.Item->InstanceId : FGuid();
		Key.BagRevision = Context.Bag ? Context.Bag->RuntimeRevision : 0;
		Key.ContextHash = BuildContextHash(Context);
		return Key;
	}

	FText BuildShopPriceLine(const TArray<FYIShopPrice>& Prices, const bool bForBuy)
	{
		if (Prices.Num() == 0)
		{
			return FText::GetEmpty();
		}

		TArray<FText> Chunks;
		Chunks.Reserve(Prices.Num());
		for (const FYIShopPrice& Price : Prices)
		{
			if (Price.Resource.IsNone() || Price.Amount <= 0)
			{
				continue;
			}

			Chunks.Add(FText::Format(
				NSLOCTEXT("YOLOInventory", "DescShopPriceChunk", "{0} {1}"),
				FText::AsNumber(Price.Amount),
				FText::FromName(Price.Resource)));
		}

		if (Chunks.Num() == 0)
		{
			return FText::GetEmpty();
		}

		FString Joined;
		for (int32 Index = 0; Index < Chunks.Num(); ++Index)
		{
			if (Index > 0)
			{
				Joined += TEXT(" + ");
			}
			Joined += Chunks[Index].ToString();
		}

		return FText::Format(
			bForBuy
				? NSLOCTEXT("YOLOInventory", "DescBuyPriceLine", "Buy Price: {0}")
				: NSLOCTEXT("YOLOInventory", "DescSellPriceLine", "Sell Price: {0}"),
			FText::FromString(Joined));
	}

	void ResolveDurability(const FYIItemInstance& Item, FYIDescriptionCacheValue& OutValue)
	{
		if (const FInstancedStruct* RuntimeDurability = Item.FindFragmentByStruct(FYIItemDurabilityRuntimeFragment::StaticStruct()))
		{
			if (const FYIItemDurabilityRuntimeFragment* Durability = RuntimeDurability->GetPtr<FYIItemDurabilityRuntimeFragment>())
			{
				if (Durability->bEnabled && Durability->Max > 0)
				{
					OutValue.bHasDurability = true;
					OutValue.CurrentDurability = static_cast<float>(Durability->Current);
					OutValue.MaxDurability = static_cast<float>(Durability->Max);
					return;
				}
			}
		}

		// Legacy fallback while older runtime payloads still exist in content.
		if (const FYIItemDurabilityFragment* LegacyDurability = YIItemInstanceFragments::GetDurability(Item))
		{
			if (LegacyDurability->bEnabled && LegacyDurability->Max > 0.0f)
			{
				OutValue.bHasDurability = true;
				OutValue.CurrentDurability = LegacyDurability->Current;
				OutValue.MaxDurability = LegacyDurability->Max;
			}
		}
	}

	void ResolveChargesAndCooldown(const FYIItemInstance& Item, FYIDescriptionCacheValue& OutValue, const APlayerState* ViewerPlayerState)
	{
		if (const FInstancedStruct* ChargesFragmentStruct = Item.FindFragmentByStruct(FYIItemChargesRuntimeFragment::StaticStruct()))
		{
			if (const FYIItemChargesRuntimeFragment* Charges = ChargesFragmentStruct->GetPtr<FYIItemChargesRuntimeFragment>())
			{
				OutValue.ExtraAffixLines.Add(FText::Format(
					NSLOCTEXT("YOLOInventory", "DescChargesLine", "Charges: {0}/{1}"),
					FText::AsNumber(FMath::Max(0, Charges->Current)),
					FText::AsNumber(FMath::Max(0, Charges->Max))));
			}
		}

		if (const FInstancedStruct* CooldownFragmentStruct = Item.FindFragmentByStruct(FYIItemCooldownRuntimeFragment::StaticStruct()))
		{
			if (const FYIItemCooldownRuntimeFragment* Cooldown = CooldownFragmentStruct->GetPtr<FYIItemCooldownRuntimeFragment>())
			{
				double RemainingSeconds = 0.0;
				if (ViewerPlayerState && ViewerPlayerState->GetWorld())
				{
					const double Now = ViewerPlayerState->GetWorld()->GetTimeSeconds();
					RemainingSeconds = (Cooldown->LastActivatedServerTime + Cooldown->CooldownDurationSeconds) - Now;
				}

				if (RemainingSeconds > 0.0)
				{
					OutValue.ExtraAffixLines.Add(FText::Format(
						NSLOCTEXT("YOLOInventory", "DescCooldownLine", "Cooldown: {0}s"),
						FText::AsNumber(FMath::RoundToInt(RemainingSeconds))));
				}
				else if (Cooldown->CooldownDurationSeconds > 0.0f)
				{
					OutValue.ExtraAffixLines.Add(NSLOCTEXT("YOLOInventory", "DescReadyLine", "Ready"));
				}
			}
		}
	}

	void ResolveTradePolicy(const FYIItemInstance& Item, FYIDescriptionCacheValue& OutValue)
	{
		const UYIItemDefinition* Definition = Item.Definition.Get();
		if (!Definition)
		{
			return;
		}

		const FYIItemSchemaSnapshot& Snapshot = YIItemSchema::ResolveSnapshot(Definition);
		if (const FYIItemTradePolicyFragment* TradePolicy = Snapshot.FindResolvedFragment<FYIItemTradePolicyFragment>())
		{
			if (!TradePolicy->bTradable)
			{
				FYITooltipRequirementLine& Line = OutValue.ExtraRequirementLines.AddDefaulted_GetRef();
				Line.Text = NSLOCTEXT("YOLOInventory", "DescNotTradable", "Not tradable");
				Line.bMet = false;
			}
			else if (!TradePolicy->bVisibleInTrade)
			{
				FYITooltipRequirementLine& Line = OutValue.ExtraRequirementLines.AddDefaulted_GetRef();
				Line.Text = NSLOCTEXT("YOLOInventory", "DescHiddenTrade", "Hidden in trade offers");
				Line.bMet = true;
			}
		}

		if (const FInstancedStruct* BindStateStruct = Item.FindFragmentByStruct(FYIItemBindStateRuntimeFragment::StaticStruct()))
		{
			if (const FYIItemBindStateRuntimeFragment* BindState = BindStateStruct->GetPtr<FYIItemBindStateRuntimeFragment>())
			{
				if (BindState->bAccountBound || BindState->bCharacterBound)
				{
					FYITooltipRequirementLine& Line = OutValue.ExtraRequirementLines.AddDefaulted_GetRef();
					Line.Text = BindState->bAccountBound
						? NSLOCTEXT("YOLOInventory", "DescAccountBound", "Account bound")
						: NSLOCTEXT("YOLOInventory", "DescCharacterBound", "Character bound");
					Line.bMet = false;
				}
			}
		}
	}

	void ResolveShopPricing(const FYIItemDescriptionContext& Context, FYIDescriptionCacheValue& OutValue)
	{
		if (!Context.Shop || !Context.Item)
		{
			return;
		}

		TArray<FYIShopPrice> Prices;
		if (!Context.Shop->ResolveDisplayPriceForItem(*Context.Item, Context.bShopBuyContext, Context.ViewerPlayerState, FMath::Max(1, Context.Count), Prices))
		{
			return;
		}

		OutValue.EconomyLine = BuildShopPriceLine(Prices, Context.bShopBuyContext);
		if (!Context.bShopBuyContext && Prices.Num() > 0)
		{
			const int64 ClampedSellPrice = FMath::Clamp<int64>(Prices[0].Amount, 0, MAX_int32);
			OutValue.SellPrice = static_cast<int32>(ClampedSellPrice);
		}
	}

	FYIDescriptionCacheValue BuildDescription(const FYIItemDescriptionContext& Context)
	{
		FYIDescriptionCacheValue Value;
		if (!Context.Item)
		{
			return Value;
		}

		ResolveDurability(*Context.Item, Value);
		ResolveChargesAndCooldown(*Context.Item, Value, Context.ViewerPlayerState);
		ResolveTradePolicy(*Context.Item, Value);
		ResolveShopPricing(Context, Value);
		Value.LastAccessCycle = static_cast<uint64>(FPlatformTime::Cycles64());
		return Value;
	}
}

void FYIItemDescriptionResolver::AugmentTooltip(const FYIItemDescriptionContext& Context, FYITooltipData& InOutTooltipData)
{
	if (!Context.Item)
	{
		return;
	}

	const FYIDescriptionCacheKey CacheKey = BuildCacheKey(Context);

	{
		FReadScopeLock ReadLock(GDescriptionCacheLock);
		if (const FYIDescriptionCacheValue* Cached = GDescriptionCache.Find(CacheKey))
		{
			if (!Cached->EconomyLine.IsEmpty())
			{
				InOutTooltipData.EconomyLine = Cached->EconomyLine;
			}
			if (Cached->SellPrice > 0)
			{
				InOutTooltipData.SellPrice = Cached->SellPrice;
			}
			if (Cached->bHasDurability)
			{
				InOutTooltipData.bHasDurability = true;
				InOutTooltipData.CurrentDurability = Cached->CurrentDurability;
				InOutTooltipData.MaxDurability = Cached->MaxDurability;
			}
			InOutTooltipData.AffixLines.Append(Cached->ExtraAffixLines);
			InOutTooltipData.RequirementLines.Append(Cached->ExtraRequirementLines);
			return;
		}
	}

	FYIDescriptionCacheValue Resolved = BuildDescription(Context);
	{
		FWriteScopeLock WriteLock(GDescriptionCacheLock);
		Resolved.LastAccessCycle = static_cast<uint64>(FPlatformTime::Cycles64());
		GDescriptionCache.Add(CacheKey, Resolved);
		YI_TrimDescriptionCacheUnsafe();
	}

	if (!Resolved.EconomyLine.IsEmpty())
	{
		InOutTooltipData.EconomyLine = Resolved.EconomyLine;
	}
	if (Resolved.SellPrice > 0)
	{
		InOutTooltipData.SellPrice = Resolved.SellPrice;
	}
	if (Resolved.bHasDurability)
	{
		InOutTooltipData.bHasDurability = true;
		InOutTooltipData.CurrentDurability = Resolved.CurrentDurability;
		InOutTooltipData.MaxDurability = Resolved.MaxDurability;
	}
	InOutTooltipData.AffixLines.Append(Resolved.ExtraAffixLines);
	InOutTooltipData.RequirementLines.Append(Resolved.ExtraRequirementLines);
}

void FYIItemDescriptionResolver::InvalidateCacheForItem(const FGuid& ItemInstanceId)
{
	if (!ItemInstanceId.IsValid())
	{
		return;
	}

	FWriteScopeLock WriteLock(GDescriptionCacheLock);
	for (auto It = GDescriptionCache.CreateIterator(); It; ++It)
	{
		if (It.Key().ItemInstanceId == ItemInstanceId)
		{
			It.RemoveCurrent();
		}
	}
}

void FYIItemDescriptionResolver::InvalidateAllCaches()
{
	FWriteScopeLock WriteLock(GDescriptionCacheLock);
	GDescriptionCache.Reset();
}
