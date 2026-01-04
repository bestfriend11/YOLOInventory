#include "YOLOInventorySettings.h"

const UYOLOInventorySettings& UYOLOInventorySettings::Get()
{
	return *GetDefault<UYOLOInventorySettings>();
}

UYOLOInventorySettings& UYOLOInventorySettings::GetMutable()
{
	return *GetMutableDefault<UYOLOInventorySettings>();
}

#if WITH_EDITOR
FName UYOLOInventorySettings::GetCategoryName() const
{
	return TEXT("Plugins");
}
#endif
