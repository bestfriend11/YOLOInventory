#include "SYIAffixDashboard.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "YIAffixAsset.h"
#include "YIAffixPoolAsset.h"
#include "YIAffixFactory.h"
#include "YIAffixPoolFactory.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#include "Misc/PackageName.h"

void SYIAffixDashboard::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailArgs;
	DetailArgs.bAllowSearch = true;
	DetailArgs.bHideSelectionTip = true;
	DetailsView = PropModule.CreateDetailView(DetailArgs);

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.40f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(6)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","AffixDash_NewAffix","New Affix"))
					.OnClicked(this, &SYIAffixDashboard::CreateAffix)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","AffixDash_NewPool","New Affix Pool"))
					.OnClicked(this, &SYIAffixDashboard::CreateAffixPool)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0,2)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				BuildAssetPicker()
			]
		]
		+ SSplitter::Slot().Value(0.60f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				DetailsView.ToSharedRef()
			]
		]
	];
}

TSharedRef<SWidget> SYIAffixDashboard::BuildAssetPicker()
{
	FAssetPickerConfig Picker;
	Picker.InitialAssetViewType = EAssetViewType::Tile;
	Picker.Filter.ClassPaths.Add(UYIAffixAsset::StaticClass()->GetClassPathName());
	Picker.Filter.ClassPaths.Add(UYIAffixPoolAsset::StaticClass()->GetClassPathName());
	Picker.bAllowNullSelection = false;
	Picker.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SYIAffixDashboard::OnAssetSelected);
	Picker.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SYIAffixDashboard::OnAssetDoubleClicked);

	FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	return CB.Get().CreateAssetPicker(Picker);
}

void SYIAffixDashboard::OnAssetSelected(const FAssetData& AssetData)
{
	if (!DetailsView.IsValid())
	{
		return;
	}
	if (UObject* Obj = AssetData.GetAsset())
	{
		DetailsView->SetObject(Obj);
	}
}

void SYIAffixDashboard::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	if (UObject* Obj = AssetData.GetAsset())
	{
		if (GEditor)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Obj);
		}
	}
}

FReply SYIAffixDashboard::CreateAffix()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UYIAffixFactory* Factory = NewObject<UYIAffixFactory>();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Affixes");
	const FString BaseName = TEXT("Affix");
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
	return FReply::Handled();
}

FReply SYIAffixDashboard::CreateAffixPool()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UYIAffixPoolFactory* Factory = NewObject<UYIAffixPoolFactory>();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Affixes");
	const FString BaseName = TEXT("AffixPool");
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
	return FReply::Handled();
}
