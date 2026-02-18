#include "YIItemInstance.h"

namespace
{
template<typename TFragment>
const TFragment* YI_FindFragmentConst(const TArray<FInstancedStruct>& Fragments)
{
	for (const FInstancedStruct& Fragment : Fragments)
	{
		if (const TFragment* Value = Fragment.GetPtr<TFragment>())
		{
			return Value;
		}
	}
	return nullptr;
}

template<typename TFragment>
TFragment* YI_FindFragmentMutable(TArray<FInstancedStruct>& Fragments, bool bCreateIfMissing)
{
	for (FInstancedStruct& Fragment : Fragments)
	{
		if (TFragment* Value = Fragment.GetMutablePtr<TFragment>())
		{
			return Value;
		}
	}

	if (!bCreateIfMissing)
	{
		return nullptr;
	}

	FInstancedStruct& NewFragment = Fragments.AddDefaulted_GetRef();
	NewFragment.InitializeAs<TFragment>();
	return NewFragment.GetMutablePtr<TFragment>();
}
}

void FYIItemInstance::SyncLegacyToCoreFragments()
{
	if (!Affixes.IsEmpty())
	{
		if (FYIItemAffixesFragment* AffixFragment = GetMutableAffixesFragment(true))
		{
			AffixFragment->Values = Affixes;
		}
	}

	if (!Attributes.IsEmpty())
	{
		if (FYIItemAttributesFragment* AttrFragment = GetMutableAttributesFragment(true))
		{
			AttrFragment->Values = Attributes;
		}
	}
}

void FYIItemInstance::SyncCoreFragmentsToLegacy()
{
	if (const FYIItemAffixesFragment* AffixFragment = GetAffixesFragment())
	{
		Affixes = AffixFragment->Values;
	}

	if (const FYIItemAttributesFragment* AttrFragment = GetAttributesFragment())
	{
		Attributes = AttrFragment->Values;
	}
}

const FYIItemAffixesFragment* FYIItemInstance::GetAffixesFragment() const
{
	return YI_FindFragmentConst<FYIItemAffixesFragment>(Fragments);
}

FYIItemAffixesFragment* FYIItemInstance::GetMutableAffixesFragment(bool bCreateIfMissing)
{
	return YI_FindFragmentMutable<FYIItemAffixesFragment>(Fragments, bCreateIfMissing);
}

const FYIItemAttributesFragment* FYIItemInstance::GetAttributesFragment() const
{
	return YI_FindFragmentConst<FYIItemAttributesFragment>(Fragments);
}

FYIItemAttributesFragment* FYIItemInstance::GetMutableAttributesFragment(bool bCreateIfMissing)
{
	return YI_FindFragmentMutable<FYIItemAttributesFragment>(Fragments, bCreateIfMissing);
}

const FYIItemDurabilityFragment* FYIItemInstance::GetDurabilityFragment() const
{
	return YI_FindFragmentConst<FYIItemDurabilityFragment>(Fragments);
}

FYIItemDurabilityFragment* FYIItemInstance::GetMutableDurabilityFragment(bool bCreateIfMissing)
{
	return YI_FindFragmentMutable<FYIItemDurabilityFragment>(Fragments, bCreateIfMissing);
}

