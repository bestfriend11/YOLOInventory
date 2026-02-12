#include "SYIBagDashboard.h"
#include "SBagEditor.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIInventoryBagFactory.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "Misc/PackageName.h"
#include "FileHelpers.h"

namespace
{
static bool YIBagDash_SaveObjectPackage(UObject* ObjectToSave)
{
	if (!ObjectToSave)
	{
		return false;
	}

	UPackage* Package = ObjectToSave->GetOutermost();
	if (!Package)
	{
		return false;
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);
	const bool bCheckDirty = false;
	const bool bPromptToSave = false;
	return FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave) == FEditorFileUtils::PR_Success;
}
}

void SYIBagDashboard::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailArgs;
	DetailArgs.bAllowSearch = true;
	DetailArgs.bHideSelectionTip = true;
	DetailsView = PropModule.CreateDetailView(DetailArgs);

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.28f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(6)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "BagDash_New", "New Bag"))
						.OnClicked(this, &SYIBagDashboard::CreateNewBag)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "BagDash_Save", "Save Bag"))
						.OnClicked(this, &SYIBagDashboard::SaveCurrentBag)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return StatusText.IsEmpty()
							? NSLOCTEXT("YOLOInventory", "BagDash_StatusDefault", "Select a bag asset to edit its runtime grid.")
							: StatusText;
					})
				]
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, 6, 0, 0)
				[
					SNew(SSplitter)
					.Orientation(Orient_Vertical)
					+ SSplitter::Slot().Value(0.50f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						.Padding(2)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(2)
							[
								SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "BagDash_BagPicker", "Bags"))
							]
							+ SVerticalBox::Slot().FillHeight(1.f)
							[
								BuildBagAssetPicker()
							]
						]
					]
					+ SSplitter::Slot().Value(0.50f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						.Padding(2)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(2)
							[
								SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "BagDash_ItemPicker", "Items (Drag to Grid)"))
							]
							+ SVerticalBox::Slot().FillHeight(1.f)
							[
								BuildItemAssetPicker()
							]
						]
					]
				]
			]
		]
		+ SSplitter::Slot().Value(0.72f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			+ SSplitter::Slot().Value(0.65f)
			[
				SAssignNew(GridHost, SBox)
			]
			+ SSplitter::Slot().Value(0.35f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(4)
				[
					DetailsView.IsValid()
						? StaticCastSharedRef<SWidget>(DetailsView.ToSharedRef())
						: StaticCastSharedRef<SWidget>(SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "BagDash_NoDetails", "Details unavailable")))
				]
			]
		]
	];

	RebuildBagView();
}

void SYIBagDashboard::OpenAsset(UObject* Asset)
{
	if (UYIInventoryBag* Bag = Cast<UYIInventoryBag>(Asset))
	{
		SetSelectedBag(Bag);
		return;
	}

	if (UYIItemDefinition* ItemDef = Cast<UYIItemDefinition>(Asset))
	{
		SelectedPaletteItem = ItemDef;
		if (DetailsView.IsValid())
		{
			DetailsView->SetObject(ItemDef);
		}
	}
}

void SYIBagDashboard::SaveCurrentBagFromToolbar()
{
	SaveCurrentBag();
}

void SYIBagDashboard::CreateBagFromToolbar()
{
	CreateNewBag();
}

void SYIBagDashboard::SetSelectedBag(UYIInventoryBag* InBag)
{
	SelectedBag = InBag;
	RebuildBagView();
}

UYIInventoryBag* SYIBagDashboard::GetSelectedBag() const
{
	return SelectedBag.Get();
}

TSharedRef<SWidget> SYIBagDashboard::BuildBagAssetPicker()
{
	FAssetPickerConfig Picker;
	Picker.InitialAssetViewType = EAssetViewType::Tile;
	Picker.Filter.ClassPaths.Add(UYIInventoryBag::StaticClass()->GetClassPathName());
	Picker.bAllowNullSelection = false;
	Picker.OnAssetSelected = FOnAssetSelected::CreateLambda([this](const FAssetData& AssetData)
	{
		SetSelectedBag(Cast<UYIInventoryBag>(AssetData.GetAsset()));
	});
	Picker.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([this](const FAssetData& AssetData)
	{
		SetSelectedBag(Cast<UYIInventoryBag>(AssetData.GetAsset()));
	});

	FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	return CB.Get().CreateAssetPicker(Picker);
}

TSharedRef<SWidget> SYIBagDashboard::BuildItemAssetPicker()
{
	FAssetPickerConfig Picker;
	Picker.InitialAssetViewType = EAssetViewType::Tile;
	Picker.Filter.ClassPaths.Add(UYIItemDefinition::StaticClass()->GetClassPathName());
	Picker.bAllowNullSelection = false;
	Picker.OnAssetSelected = FOnAssetSelected::CreateLambda([this](const FAssetData& AssetData)
	{
		SelectedPaletteItem = Cast<UYIItemDefinition>(AssetData.GetAsset());
		if (DetailsView.IsValid() && SelectedPaletteItem.IsValid())
		{
			DetailsView->SetObject(SelectedPaletteItem.Get());
		}
	});
	Picker.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([this](const FAssetData& AssetData)
	{
		SelectedPaletteItem = Cast<UYIItemDefinition>(AssetData.GetAsset());
		if (DetailsView.IsValid() && SelectedPaletteItem.IsValid())
		{
			DetailsView->SetObject(SelectedPaletteItem.Get());
		}
	});

	FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	return CB.Get().CreateAssetPicker(Picker);
}

void SYIBagDashboard::RebuildBagView()
{
	if (!GridHost.IsValid())
	{
		return;
	}

	GridWidget.Reset();

	if (UYIInventoryBag* Bag = SelectedBag.Get())
	{
		GridHost->SetContent(
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(4)
			[
				SAssignNew(GridWidget, SBagEditor)
				.Bag(Bag)
			]
		);
		if (DetailsView.IsValid())
		{
			DetailsView->SetObject(Bag);
		}
	}
	else
	{
		GridHost->SetContent(
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(10)
			[
				SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "BagDash_SelectBag", "Select an Inventory Bag asset from the left panel."))
			]
		);
		if (DetailsView.IsValid())
		{
			DetailsView->SetObject(nullptr);
		}
	}
}

FReply SYIBagDashboard::SaveCurrentBag()
{
	UYIInventoryBag* Bag = SelectedBag.Get();
	if (!Bag)
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_StatusNoBag", "No bag selected.");
		return FReply::Handled();
	}

	if (YIBagDash_SaveObjectPackage(Bag))
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_StatusSaved", "Bag saved.");
	}
	else
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_StatusSaveFail", "Save canceled or failed.");
	}
	return FReply::Handled();
}

FReply SYIBagDashboard::CreateNewBag()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UYIInventoryBagFactory* Factory = NewObject<UYIInventoryBagFactory>();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Bags");
	const FString BaseName = TEXT("InventoryBag");
	FString PackageName;
	FString AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	UObject* NewAsset = Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
	SetSelectedBag(Cast<UYIInventoryBag>(NewAsset));
	StatusText = NewAsset
		? NSLOCTEXT("YOLOInventory", "BagDash_StatusCreated", "New bag created.")
		: NSLOCTEXT("YOLOInventory", "BagDash_StatusCreateFail", "Failed to create bag.");
	return FReply::Handled();
}
