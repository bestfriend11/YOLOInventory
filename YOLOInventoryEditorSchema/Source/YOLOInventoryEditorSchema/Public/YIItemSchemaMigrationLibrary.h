#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YIItemSchemaMigrationLibrary.generated.h"

/** Editor-only schema utility helpers. */
UCLASS()
class YOLOINVENTORYEDITORSCHEMA_API UYIItemSchemaMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Legacy baseline migration was removed; this now returns 0 and leaves assets unchanged.
	 * @param bSaveModifiedPackages When true, prompts save dialog for changed packages.
	 * @param OutModifiedAssets Output list of changed item-definition asset paths (always empty).
	 * @return Number of item definitions that changed (always zero).
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="YOLOInventory|Schema|Migration")
	static int32 MigrateAllItemDefinitionsToBaselineFragments(bool bSaveModifiedPackages, TArray<FSoftObjectPath>& OutModifiedAssets);
};
