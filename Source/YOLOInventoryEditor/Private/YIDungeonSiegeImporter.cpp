#include "YIDungeonSiegeImporter.h"
#include "YIItemDefinition.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemGenerator.h"
#include "YIItemDefinitionFactory.h"
#include "YILootTableFactory.h"
#include "YIRarityProfileFactory.h"
#include "YIItemGeneratorFactory.h"
#include "YIInventoryBlueprintLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"

static FString NormalizeOutputPath(const FString& InPath)
{
	if (InPath.IsEmpty())
	{
		return TEXT("/Game");
	}
	if (InPath.StartsWith(TEXT("/Game")))
	{
		return InPath;
	}
	return TEXT("/Game/") + InPath;
}

static FString MakeDisplayName(const FString& TemplateId)
{
	FString Name = TemplateId;
	Name.ReplaceInline(TEXT("_"), TEXT(" "));
	Name.ReplaceInline(TEXT("-"), TEXT(" "));
	return Name;
}

template<typename T>
static T* CreateOrLoadAsset(const FString& PackagePath, const FString& AssetName, UFactory* Factory, bool bOverwrite)
{
	const FString PackageName = PackagePath / AssetName;
	const FString ObjectPath = PackageName + TEXT(".") + AssetName;

	if (UObject* ExistingObj = StaticFindObject(nullptr, nullptr, *ObjectPath))
	{
		if (!bOverwrite)
		{
			return Cast<T>(ExistingObj);
		}
	}

	FAssetRegistryModule& Arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData Existing = Arm.Get().GetAssetByObjectPath(*ObjectPath);
	if (Existing.IsValid())
	{
		if (!bOverwrite)
		{
			return Cast<T>(Existing.GetAsset());
		}
		return Cast<T>(Existing.GetAsset());
	}

	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UObject* NewAsset = Tools.CreateAsset(AssetName, PackagePath, T::StaticClass(), Factory);
	return Cast<T>(NewAsset);
}

static bool ExtractTemplateName(const FString& Line, FString& OutName)
{
	OutName.Reset();
	int32 TemplatePos = Line.Find(TEXT("t:template"), ESearchCase::IgnoreCase, ESearchDir::FromStart);
	if (TemplatePos == INDEX_NONE)
	{
		return false;
	}
	int32 NamePos = Line.Find(TEXT("n:"), ESearchCase::IgnoreCase, ESearchDir::FromStart);
	if (NamePos == INDEX_NONE)
	{
		return false;
	}
	const int32 EndPos = Line.Find(TEXT("]"), ESearchCase::IgnoreCase, ESearchDir::FromStart, NamePos);
	if (EndPos == INDEX_NONE || EndPos <= NamePos + 2)
	{
		return false;
	}
	FString Raw = Line.Mid(NamePos + 2, EndPos - (NamePos + 2));
	Raw.TrimStartAndEndInline();
	if (Raw.IsEmpty())
	{
		return false;
	}
	OutName = Raw;
	return true;
}

int32 UYIDungeonSiegeImporter::ImportTemplatesFromGas(const FYIDungeonSiegeImportOptions& Options)
{
	if (Options.GasFilePath.IsEmpty())
	{
		return 0;
	}

	FString GasPath = Options.GasFilePath;
	FPaths::NormalizeFilename(GasPath);
	if (!FPaths::FileExists(GasPath))
	{
		return 0;
	}

	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *GasPath))
	{
		return 0;
	}

	TArray<FString> Lines;
	FileContents.ParseIntoArrayLines(Lines);

	TArray<FString> TemplateNames;
	TemplateNames.Reserve(1024);
	TSet<FString> Seen;
	for (const FString& Line : Lines)
	{
		FString TemplateName;
		if (ExtractTemplateName(Line, TemplateName))
		{
			if (!Seen.Contains(TemplateName))
			{
				Seen.Add(TemplateName);
				TemplateNames.Add(TemplateName);
			}
		}
	}

	if (TemplateNames.Num() == 0)
	{
		return 0;
	}

	const FString OutputPath = NormalizeOutputPath(Options.OutputPath);
	UYIItemDefinitionFactory* DefFactory = Options.bCreateItemDefinitions ? NewObject<UYIItemDefinitionFactory>() : nullptr;
	UYILootTableFactory* LootFactory = Options.bCreateLootTable ? NewObject<UYILootTableFactory>() : nullptr;
	UYIRarityProfileFactory* RarityFactory = Options.bCreateRarityProfile ? NewObject<UYIRarityProfileFactory>() : nullptr;
	UYIItemGeneratorFactory* GeneratorFactory = Options.bCreateItemGenerator ? NewObject<UYIItemGeneratorFactory>() : nullptr;

	TArray<TSoftObjectPtr<UYIItemDefinition>> CreatedDefs;
	int32 CreatedCount = 0;
	for (const FString& TemplateName : TemplateNames)
	{
		if (Options.MaxAssetsToCreate > 0 && CreatedCount >= Options.MaxAssetsToCreate)
		{
			break;
		}

		const FString AssetName = ObjectTools::SanitizeObjectName(TemplateName);
		if (AssetName.IsEmpty())
		{
			continue;
		}

		UYIItemDefinition* Def = nullptr;
		if (Options.bCreateItemDefinitions && DefFactory)
		{
			Def = CreateOrLoadAsset<UYIItemDefinition>(OutputPath, AssetName, DefFactory, Options.bOverwriteExisting);
			if (Def)
			{
				Def->TemplateId = TemplateName;
				Def->DisplayName = FText::FromString(MakeDisplayName(TemplateName));
				Def->Modify();
				CreatedDefs.Add(Def);
				++CreatedCount;
			}
		}
		else
		{
			if (UYIItemDefinition* Existing = UYIInventoryBlueprintLibrary::FindItemDefinitionByTemplateId(TemplateName))
			{
				CreatedDefs.Add(Existing);
			}
		}
	}

	// Create a simple loot table that includes every imported definition equally.
	UYILootTable* LootTable = nullptr;
	if (Options.bCreateLootTable && LootFactory)
	{
		const FString LootName = ObjectTools::SanitizeObjectName(Options.LootTableAssetName.ToString());
		LootTable = CreateOrLoadAsset<UYILootTable>(OutputPath, LootName, LootFactory, Options.bOverwriteExisting);
		if (LootTable)
		{
			LootTable->Entries.Reset();
			for (const TSoftObjectPtr<UYIItemDefinition>& DefSoft : CreatedDefs)
			{
				FYILootTableEntry Entry;
				Entry.Definition = DefSoft;
				Entry.Weight = 1.f;
				LootTable->Entries.Add(Entry);
			}
			LootTable->Modify();
		}
	}

	// Create a starter rarity profile with common/magic/rare buckets (weights can be adjusted later).
	UYIRarityProfile* RarityProfile = nullptr;
	if (Options.bCreateRarityProfile && RarityFactory)
	{
		const FString RarityName = ObjectTools::SanitizeObjectName(Options.RarityProfileAssetName.ToString());
		RarityProfile = CreateOrLoadAsset<UYIRarityProfile>(OutputPath, RarityName, RarityFactory, Options.bOverwriteExisting);
		if (RarityProfile)
		{
			RarityProfile->Rules.Reset();
			FYIRarityRule Common; Common.Weight = 70.f; Common.MinPrefixes = 0; Common.MaxPrefixes = 0; Common.MinSuffixes = 0; Common.MaxSuffixes = 0;
			FYIRarityRule Magic; Magic.Weight = 25.f; Magic.MinPrefixes = 1; Magic.MaxPrefixes = 1; Magic.MinSuffixes = 1; Magic.MaxSuffixes = 1;
			FYIRarityRule Rare; Rare.Weight = 5.f; Rare.MinPrefixes = 2; Rare.MaxPrefixes = 3; Rare.MinSuffixes = 2; Rare.MaxSuffixes = 3;
			RarityProfile->Rules.Add(Common);
			RarityProfile->Rules.Add(Magic);
			RarityProfile->Rules.Add(Rare);
			RarityProfile->Modify();
		}
	}

	// Create a generator that wires the loot table + rarity profile.
	if (Options.bCreateItemGenerator && GeneratorFactory)
	{
		const FString GenName = ObjectTools::SanitizeObjectName(Options.ItemGeneratorAssetName.ToString());
		UYIItemGenerator* Generator = CreateOrLoadAsset<UYIItemGenerator>(OutputPath, GenName, GeneratorFactory, Options.bOverwriteExisting);
		if (Generator)
		{
			Generator->LootTable = LootTable;
			Generator->RarityProfile = RarityProfile;
			Generator->Modify();
		}
	}

	return CreatedCount;
}
