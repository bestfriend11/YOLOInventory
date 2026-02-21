#include "YIItemSchemaMigrationLibrary.h"

int32 UYIItemSchemaMigrationLibrary::MigrateAllItemDefinitionsToBaselineFragments(bool bSaveModifiedPackages, TArray<FSoftObjectPath>& OutModifiedAssets)
{
	(void)bSaveModifiedPackages;
	OutModifiedAssets.Reset();
	return 0;
}
