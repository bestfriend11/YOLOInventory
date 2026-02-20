#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YIItemSchemaMigrationLibrary.generated.h"

/**
 * Editor-only migration helpers that normalize legacy item-definition fields into baseline fragments.
 */
UCLASS()
class YOLOINVENTORYEDITORSCHEMA_API UYIItemSchemaMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Migrate all item definitions found in the asset registry to baseline fragment data.
	 * @param bSaveModifiedPackages When true, prompts save dialog for changed packages.
	 * @param OutModifiedAssets Output list of migrated item-definition asset paths.
	 * @return Number of item definitions that changed.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="YOLOInventory|Schema|Migration")
	static int32 MigrateAllItemDefinitionsToBaselineFragments(bool bSaveModifiedPackages, TArray<FSoftObjectPath>& OutModifiedAssets);
};
