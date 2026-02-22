#include "YIItemInstance.h"

const FInstancedStruct* FYIItemInstance::FindFragmentByStruct(const UScriptStruct* FragmentStruct) const
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

FInstancedStruct* FYIItemInstance::FindMutableFragmentByStruct(const UScriptStruct* FragmentStruct, bool bCreateIfMissing)
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (FInstancedStruct& Fragment : Fragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}

	if (!bCreateIfMissing)
	{
		return nullptr;
	}

	FInstancedStruct& NewFragment = Fragments.AddDefaulted_GetRef();
	NewFragment.InitializeAs(FragmentStruct);
	return &NewFragment;
}

const FYIItemCustomRuntimeFragment* FYIItemInstance::FindCustomRuntimeFragmentByTag(FGameplayTag FragmentTag) const
{
	if (!FragmentTag.IsValid())
	{
		return nullptr;
	}

	for (const FInstancedStruct& Fragment : Fragments)
	{
		if (const FYIItemCustomRuntimeFragment* Custom = Fragment.GetPtr<FYIItemCustomRuntimeFragment>())
		{
			if (Custom->FragmentTag == FragmentTag)
			{
				return Custom;
			}
		}
	}
	return nullptr;
}

FYIItemCustomRuntimeFragment* FYIItemInstance::FindMutableCustomRuntimeFragmentByTag(FGameplayTag FragmentTag)
{
	if (!FragmentTag.IsValid())
	{
		return nullptr;
	}

	for (FInstancedStruct& Fragment : Fragments)
	{
		if (FYIItemCustomRuntimeFragment* Custom = Fragment.GetMutablePtr<FYIItemCustomRuntimeFragment>())
		{
			if (Custom->FragmentTag == FragmentTag)
			{
				return Custom;
			}
		}
	}
	return nullptr;
}

