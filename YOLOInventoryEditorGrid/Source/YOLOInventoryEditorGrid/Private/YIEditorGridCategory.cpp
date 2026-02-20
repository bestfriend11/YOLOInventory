#include "YIEditorGridCategory.h"

namespace
{
uint32 GYOLOInventoryEditorGridCategory = EAssetTypeCategories::Misc;
}

uint32 YIEditorGridCategory::Get()
{
	return GYOLOInventoryEditorGridCategory;
}

void YIEditorGridCategory::Set(uint32 InCategoryBit)
{
	GYOLOInventoryEditorGridCategory = InCategoryBit;
}

