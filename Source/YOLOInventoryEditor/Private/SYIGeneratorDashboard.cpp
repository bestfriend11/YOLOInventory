#include "SYIGeneratorDashboard.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemGenerator.h"
#include "YIAffixAsset.h"
#include "YILootTableFactory.h"
#include "YIRarityProfileFactory.h"
#include "YIItemGeneratorFactory.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#include "Misc/PackageName.h"

void SYIGeneratorDashboard::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailArgs;
	DetailArgs.bAllowSearch = true;
	DetailArgs.bHideSelectionTip = true;
	DetailsView = PropModule.CreateDetailView(DetailArgs);

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.42f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(6)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","GenDash_NewLoot","New Loot Table"))
					.OnClicked(this, &SYIGeneratorDashboard::CreateLootTable)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","GenDash_NewRarity","New Rarity Profile"))
					.OnClicked(this, &SYIGeneratorDashboard::CreateRarityProfile)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","GenDash_NewGenerator","New Item Generator"))
					.OnClicked(this, &SYIGeneratorDashboard::CreateItemGenerator)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0,2)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(6)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_Level","Level"))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SNumericEntryBox<int32>)
					.MinValue(1)
					.MaxValue(9999)
					.Value_Lambda([this]()->TOptional<int32>{ return TestLevel; })
					.OnValueChanged_Lambda([this](int32 V){ TestLevel = V; })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8,2,2,2)
				[
					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_Seed","Seed"))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SNumericEntryBox<int32>)
					.MinValue(1)
					.MaxValue(2147483647)
					.Value_Lambda([this]()->TOptional<int32>{ return TestSeed; })
					.OnValueChanged_Lambda([this](int32 V){ TestSeed = V; })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8,2,2,2)
				[
					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_Runs","Runs"))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SNumericEntryBox<int32>)
					.MinValue(1)
					.MaxValue(200)
					.Value_Lambda([this]()->TOptional<int32>{ return TestRuns; })
					.OnValueChanged_Lambda([this](int32 V){ TestRuns = FMath::Clamp(V, 1, 200); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8,2,2,2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","GenDash_Test","Generate"))
					.OnClicked(this, &SYIGeneratorDashboard::RunGeneratorTest)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(6)
			[
				SNew(STextBlock)
				.Text_Lambda([this](){ return TestResult.IsEmpty() ? NSLOCTEXT("YOLOInventory","GenDash_NoResult","Select a generator and click Generate.") : TestResult; })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0,4)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				BuildAssetPicker()
			]
		]
		+ SSplitter::Slot().Value(0.58f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				DetailsView.ToSharedRef()
			]
		]
	];
}

TSharedRef<SWidget> SYIGeneratorDashboard::BuildAssetPicker()
{
	FAssetPickerConfig Picker;
	Picker.InitialAssetViewType = EAssetViewType::Tile;
	Picker.Filter.ClassPaths.Add(UYIItemGenerator::StaticClass()->GetClassPathName());
	Picker.Filter.ClassPaths.Add(UYILootTable::StaticClass()->GetClassPathName());
	Picker.Filter.ClassPaths.Add(UYIRarityProfile::StaticClass()->GetClassPathName());
	Picker.bAllowNullSelection = false;
	Picker.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SYIGeneratorDashboard::OnAssetSelected);
	Picker.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SYIGeneratorDashboard::OnAssetDoubleClicked);

	FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	return CB.Get().CreateAssetPicker(Picker);
}

void SYIGeneratorDashboard::OnAssetSelected(const FAssetData& AssetData)
{
	SelectedAsset = AssetData.GetAsset();
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(SelectedAsset.Get());
	}
}

void SYIGeneratorDashboard::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	OnAssetSelected(AssetData);
}

void SYIGeneratorDashboard::OpenAsset(UObject* Asset)
{
	if (!DetailsView.IsValid())
	{
		return;
	}
	if (Asset)
	{
		SelectedAsset = Asset;
		DetailsView->SetObject(Asset);
	}
}

FReply SYIGeneratorDashboard::CreateLootTable()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UYILootTableFactory* Factory = NewObject<UYILootTableFactory>();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Loot");
	const FString BaseName = TEXT("LootTable");
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
	return FReply::Handled();
}

FReply SYIGeneratorDashboard::CreateRarityProfile()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UYIRarityProfileFactory* Factory = NewObject<UYIRarityProfileFactory>();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Rarity");
	const FString BaseName = TEXT("RarityProfile");
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
	return FReply::Handled();
}

FReply SYIGeneratorDashboard::CreateItemGenerator()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UYIItemGeneratorFactory* Factory = NewObject<UYIItemGeneratorFactory>();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Generators");
	const FString BaseName = TEXT("ItemGenerator");
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
	return FReply::Handled();
}

FReply SYIGeneratorDashboard::RunGeneratorTest()
{
	TestResult = FText::GetEmpty();
	UYIItemGenerator* Generator = Cast<UYIItemGenerator>(SelectedAsset.Get());
	if (!Generator)
	{
		TestResult = NSLOCTEXT("YOLOInventory","GenDash_SelectGenerator","Select an Item Generator asset first.");
		return FReply::Handled();
	}

	FYIBagItem Item;
	FGameplayTag Rarity;
	int32 Prefixes = 0;
	int32 Suffixes = 0;

	const int32 NumRuns = FMath::Max(1, TestRuns);
	int32 SuccessCount = 0;
	int32 FailureCount = 0;
	TMap<FString, int32> RarityHistogram;
	int64 PrefixTotal = 0;
	int64 SuffixTotal = 0;

	if (NumRuns == 1)
	{
		if (!Generator->GenerateItem(TestLevel, TestSeed, Item, Rarity, Prefixes, Suffixes))
		{
			TestResult = NSLOCTEXT("YOLOInventory","GenDash_GenerateFailed","Generator failed to roll an item. Check loot table and definitions.");
			return FReply::Handled();
		}
		SuccessCount = 1;
		PrefixTotal = Prefixes;
		SuffixTotal = Suffixes;
		const FString RarityName = Rarity.IsValid() ? Rarity.ToString() : TEXT("None");
		RarityHistogram.FindOrAdd(RarityName)++;
	}
	else
	{
		for (int32 RunIndex = 0; RunIndex < NumRuns; ++RunIndex)
		{
			const int32 RunSeed = TestSeed + RunIndex;
			FYIBagItem TmpItem;
			FGameplayTag TmpRarity;
			int32 TmpPrefixes = 0;
			int32 TmpSuffixes = 0;
			if (Generator->GenerateItem(TestLevel, RunSeed, TmpItem, TmpRarity, TmpPrefixes, TmpSuffixes))
			{
				SuccessCount++;
				PrefixTotal += TmpPrefixes;
				SuffixTotal += TmpSuffixes;
				const FString RarityName = TmpRarity.IsValid() ? TmpRarity.ToString() : TEXT("None");
				RarityHistogram.FindOrAdd(RarityName)++;
				if (RunIndex == 0)
				{
					Item = TmpItem;
					Rarity = TmpRarity;
					Prefixes = TmpPrefixes;
					Suffixes = TmpSuffixes;
				}
			}
			else
			{
				FailureCount++;
			}
		}
		if (SuccessCount == 0)
		{
			TestResult = NSLOCTEXT("YOLOInventory","GenDash_GenerateFailed","Generator failed to roll an item. Check loot table and definitions.");
			return FReply::Handled();
		}
	}

	FString DefName = TEXT("Unknown");
	if (UYIItemDefinition* Def = Item.Item.Definition.IsValid() ? Item.Item.Definition.Get() : Item.Item.Definition.LoadSynchronous())
	{
		DefName = Def->DisplayName.IsEmpty() ? Def->GetName() : Def->DisplayName.ToString();
	}
	const FString RarityName = Rarity.IsValid() ? Rarity.ToString() : TEXT("None");

	auto CriteriaSummary = [](const FYIAffixRollCriteria& Criteria)->FString
	{
		if (!Criteria.bEnabled)
		{
			return TEXT("Disabled");
		}
		const FString PowerWindow = Criteria.bUseItemLevelAsPowerBaseline
			? FString::Printf(TEXT("Lvl+[%d..%d]"), Criteria.MinPowerLevelOffset, Criteria.MaxPowerLevelOffset)
			: FString::Printf(TEXT("%d..%d"), Criteria.MinPowerLevel, Criteria.MaxPowerLevel);
		return FString::Printf(
			TEXT("Tier[%d..%d], Power[%s], Quality[%d..%d]"),
			Criteria.MinTier, Criteria.MaxTier, *PowerWindow, (int32)Criteria.MinQuality, (int32)Criteria.MaxQuality);
	};

	FString Detail;
	Detail += FString::Printf(TEXT("Item: %s x%d | Rarity: %s | Prefixes: %d | Suffixes: %d"),
		*DefName, Item.Item.Count, *RarityName, Prefixes, Suffixes);
	Detail += LINE_TERMINATOR;
	Detail += FString::Printf(TEXT("Runs: %d | Success: %d | Failed: %d | AvgPrefixes: %.2f | AvgSuffixes: %.2f"),
		NumRuns, SuccessCount, FailureCount,
		SuccessCount > 0 ? (double)PrefixTotal / (double)SuccessCount : 0.0,
		SuccessCount > 0 ? (double)SuffixTotal / (double)SuccessCount : 0.0);
	if (RarityHistogram.Num() > 0)
	{
		Detail += LINE_TERMINATOR;
		Detail += TEXT("Rarity Distribution:");
		for (const TPair<FString, int32>& Pair : RarityHistogram)
		{
			Detail += LINE_TERMINATOR;
			Detail += FString::Printf(TEXT("- %s: %d"), *Pair.Key, Pair.Value);
		}
	}
	Detail += LINE_TERMINATOR;
	Detail += FString::Printf(TEXT("Prefix Criteria: %s"), *CriteriaSummary(Generator->PrefixCriteria));
	Detail += LINE_TERMINATOR;
	Detail += FString::Printf(TEXT("Suffix Criteria: %s"), *CriteriaSummary(Generator->SuffixCriteria));

	if (Item.Item.Affixes.Num() > 0)
	{
		Detail += LINE_TERMINATOR;
		Detail += TEXT("Rolled Affixes:");
		for (const FYIAffixInstance& Inst : Item.Item.Affixes)
		{
			const UYIAffixAsset* Src = Inst.Source.IsValid() ? Inst.Source.Get() : Inst.Source.LoadSynchronous();
			const FString Name = Src && !Src->DisplayName.IsEmpty() ? Src->DisplayName.ToString() : Inst.DisplayNameCache.ToString();
			const FString TemplateId = Src ? Src->TemplateId : FString();
			const int32 Tier = Src ? Src->Tier : Inst.TierRolled;
			const int32 Power = Src ? Src->PowerLevel : 0;
			const int32 Kind = Src ? (int32)Src->Kind : -1;
			Detail += LINE_TERMINATOR;
			Detail += FString::Printf(
				TEXT("- %s [kind=%d tier=%d power=%d roll=%.2f template=%s]"),
				*Name, Kind, Tier, Power, Inst.RolledValue, *TemplateId);
		}
	}

	TestResult = FText::FromString(Detail);
	return FReply::Handled();
}
