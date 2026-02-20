#include "YIItemTraitAsset.h"
#include "YIItemSchemaResolver.h"
#include "UObject/UnrealType.h"

const FInstancedStruct* UYIItemTraitAsset::FindDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (const FInstancedStruct& Fragment : DefinitionFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}
	return nullptr;
}

FInstancedStruct* UYIItemTraitAsset::FindOrAddDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct)
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (FInstancedStruct& Fragment : DefinitionFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}

	FInstancedStruct& NewFragment = DefinitionFragments.AddDefaulted_GetRef();
	NewFragment.InitializeAs(FragmentStruct);
	return &NewFragment;
}

#if WITH_EDITOR
void UYIItemTraitAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	YIItemSchema::InvalidateAllSnapshotCaches();
}
#endif
