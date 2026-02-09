#include "SYIGeneratorDashboard.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemGenerator.h"
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
	if (UObject* Obj = AssetData.GetAsset())
	{
		if (GEditor)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Obj);
		}
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
	if (!Generator->GenerateItem(TestLevel, TestSeed, Item, Rarity, Prefixes, Suffixes))
	{
		TestResult = NSLOCTEXT("YOLOInventory","GenDash_GenerateFailed","Generator failed to roll an item. Check loot table and definitions.");
		return FReply::Handled();
	}

	FString DefName = TEXT("Unknown");
	if (UYIItemDefinition* Def = Item.Item.Definition.IsValid() ? Item.Item.Definition.Get() : Item.Item.Definition.LoadSynchronous())
	{
		DefName = Def->DisplayName.IsEmpty() ? Def->GetName() : Def->DisplayName.ToString();
	}
	const FString RarityName = Rarity.IsValid() ? Rarity.ToString() : TEXT("None");
	TestResult = FText::FromString(FString::Printf(TEXT("Item: %s x%d | Rarity: %s | Prefixes: %d | Suffixes: %d"),
		*DefName, Item.Item.Count, *RarityName, Prefixes, Suffixes));
	return FReply::Handled();
}
