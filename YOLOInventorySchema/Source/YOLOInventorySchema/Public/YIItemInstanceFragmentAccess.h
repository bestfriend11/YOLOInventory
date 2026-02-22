#pragma once

#include "CoreMinimal.h"
#include "YIItemFragments.h"
#include "YIItemInstance.h"
#include "YIItemNetTypes.h"

namespace YIItemInstanceFragments
{
	inline const FYIItemAffixesFragment* GetAffixes(const FYIItemInstance& Item)
	{
		if (const FInstancedStruct* Fragment = Item.FindFragmentByStruct(FYIItemAffixesFragment::StaticStruct()))
		{
			return Fragment->GetPtr<FYIItemAffixesFragment>();
		}
		return nullptr;
	}

	inline FYIItemAffixesFragment* GetMutableAffixes(FYIItemInstance& Item, bool bCreateIfMissing)
	{
		if (FInstancedStruct* Fragment = Item.FindMutableFragmentByStruct(FYIItemAffixesFragment::StaticStruct(), bCreateIfMissing))
		{
			return Fragment->GetMutablePtr<FYIItemAffixesFragment>();
		}
		return nullptr;
	}

	inline const FYIItemAttributesFragment* GetAttributes(const FYIItemInstance& Item)
	{
		if (const FInstancedStruct* Fragment = Item.FindFragmentByStruct(FYIItemAttributesFragment::StaticStruct()))
		{
			return Fragment->GetPtr<FYIItemAttributesFragment>();
		}
		return nullptr;
	}

	inline FYIItemAttributesFragment* GetMutableAttributes(FYIItemInstance& Item, bool bCreateIfMissing)
	{
		if (FInstancedStruct* Fragment = Item.FindMutableFragmentByStruct(FYIItemAttributesFragment::StaticStruct(), bCreateIfMissing))
		{
			return Fragment->GetMutablePtr<FYIItemAttributesFragment>();
		}
		return nullptr;
	}

	inline const FYIItemDurabilityFragment* GetDurability(const FYIItemInstance& Item)
	{
		if (const FInstancedStruct* Fragment = Item.FindFragmentByStruct(FYIItemDurabilityFragment::StaticStruct()))
		{
			return Fragment->GetPtr<FYIItemDurabilityFragment>();
		}
		return nullptr;
	}

	inline FYIItemDurabilityFragment* GetMutableDurability(FYIItemInstance& Item, bool bCreateIfMissing)
	{
		if (FInstancedStruct* Fragment = Item.FindMutableFragmentByStruct(FYIItemDurabilityFragment::StaticStruct(), bCreateIfMissing))
		{
			return Fragment->GetMutablePtr<FYIItemDurabilityFragment>();
		}
		return nullptr;
	}

	inline void ExportLegacyNetPayload(const FYIItemInstance& Item, TArray<FYIAffixInstance>& OutAffixes, TArray<FYIAttributeKV>& OutAttributes)
	{
		OutAffixes.Reset();
		OutAttributes.Reset();

		if (const FYIItemAffixesFragment* Affixes = GetAffixes(Item))
		{
			OutAffixes = Affixes->Values;
		}

		if (const FYIItemAttributesFragment* Attributes = GetAttributes(Item))
		{
			OutAttributes.Reserve(Attributes->Values.Num());
			for (const TPair<FName, float>& KV : Attributes->Values)
			{
				FYIAttributeKV OutKV;
				OutKV.Name = KV.Key;
				OutKV.Value = KV.Value;
				OutAttributes.Add(OutKV);
			}
		}
	}

	inline void ImportLegacyNetPayload(FYIItemInstance& Item, const TArray<FYIAffixInstance>& InAffixes, const TArray<FYIAttributeKV>& InAttributes)
	{
		if (InAffixes.Num() > 0)
		{
			if (FYIItemAffixesFragment* Affixes = GetMutableAffixes(Item, true))
			{
				Affixes->Values = InAffixes;
			}
		}

		if (InAttributes.Num() > 0)
		{
			if (FYIItemAttributesFragment* Attributes = GetMutableAttributes(Item, true))
			{
				Attributes->Values.Reset();
				for (const FYIAttributeKV& KV : InAttributes)
				{
					Attributes->Values.Add(KV.Name, KV.Value);
				}
			}
		}
	}
}

