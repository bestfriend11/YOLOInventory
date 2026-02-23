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

	inline void ExportNetFragmentPayload(const FYIItemInstance& Item, TArray<FInstancedStruct>& OutFragments)
	{
		OutFragments = Item.Fragments;
	}

	inline void ImportNetFragmentPayload(FYIItemInstance& Item, const TArray<FInstancedStruct>& InFragments)
	{
		Item.Fragments = InFragments;
	}
}
