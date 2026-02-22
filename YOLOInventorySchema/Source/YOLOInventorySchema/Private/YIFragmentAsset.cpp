#include "YIFragmentAsset.h"

const FInstancedStruct* UYIFragmentAsset::FindItemDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (const FInstancedStruct& Fragment : ItemDefinitionFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}
	return nullptr;
}

FInstancedStruct* UYIFragmentAsset::FindOrAddItemDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct)
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (FInstancedStruct& Fragment : ItemDefinitionFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}

	FInstancedStruct& NewFragment = ItemDefinitionFragments.AddDefaulted_GetRef();
	NewFragment.InitializeAs(FragmentStruct);
	return &NewFragment;
}

const FInstancedStruct* UYIFragmentAsset::FindItemInstanceFragmentByStruct(const UScriptStruct* FragmentStruct) const
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (const FInstancedStruct& Fragment : ItemInstanceFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}
	return nullptr;
}

FInstancedStruct* UYIFragmentAsset::FindOrAddItemInstanceFragmentByStruct(const UScriptStruct* FragmentStruct)
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (FInstancedStruct& Fragment : ItemInstanceFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}

	FInstancedStruct& NewFragment = ItemInstanceFragments.AddDefaulted_GetRef();
	NewFragment.InitializeAs(FragmentStruct);
	return &NewFragment;
}

const FInstancedStruct* UYIFragmentAsset::FindAffixDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (const FInstancedStruct& Fragment : AffixDefinitionFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}
	return nullptr;
}

FInstancedStruct* UYIFragmentAsset::FindOrAddAffixDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct)
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (FInstancedStruct& Fragment : AffixDefinitionFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}

	FInstancedStruct& NewFragment = AffixDefinitionFragments.AddDefaulted_GetRef();
	NewFragment.InitializeAs(FragmentStruct);
	return &NewFragment;
}
