#pragma once

#include "CoreMinimal.h"
#include "IAssetTools.h"

class YIEditorLootCategory
{
public:
	static void Set(uint32 InCategory)
	{
		GetMutableCategory() = InCategory;
	}

	static uint32 Get()
	{
		return GetMutableCategory();
	}

private:
	static uint32& GetMutableCategory()
	{
		static uint32 CategoryBit = EAssetTypeCategories::Misc;
		return CategoryBit;
	}
};
