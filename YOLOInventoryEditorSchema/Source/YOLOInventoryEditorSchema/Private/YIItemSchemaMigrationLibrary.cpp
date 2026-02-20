#include "YIItemSchemaMigrationLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "Modules/ModuleManager.h"
#include "YIItemDefinition.h"

int32 UYIItemSchemaMigrationLibrary::MigrateAllItemDefinitionsToBaselineFragments(bool bSaveModifiedPackages, TArray<FSoftObjectPath>& OutModifiedAssets)
{
	OutModifiedAssets.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(UYIItemDefinition::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Reserve(Assets.Num());

	for (const FAssetData& AssetData : Assets)
	{
		UYIItemDefinition* ItemDef = Cast<UYIItemDefinition>(AssetData.GetAsset());
		if (!ItemDef)
		{
			continue;
		}

		ItemDef->Modify();
		if (!ItemDef->EnsureBaselineDefinitionFragments())
		{
			continue;
		}

		ItemDef->MarkPackageDirty();
		OutModifiedAssets.Add(AssetData.ToSoftObjectPath());
		if (UPackage* Package = ItemDef->GetOutermost())
		{
			PackagesToSave.AddUnique(Package);
		}
	}

	if (bSaveModifiedPackages && PackagesToSave.Num() > 0)
	{
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, true, true);
	}

	return OutModifiedAssets.Num();
}
