#include "SYIBagDashboard.h"
#include "SBagEditor.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIInventoryGameplaySetupLibrary.h"
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
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Styling/AppStyle.h"
#include "Misc/PackageName.h"
#include "FileHelpers.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

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

static APawn* YIBagDash_ResolveRuntimePawn()
{
	if (!GEditor || !GEditor->PlayWorld)
	{
		return nullptr;
	}

	UWorld* PlayWorld = GEditor->PlayWorld;
	if (!PlayWorld)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = PlayWorld->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				return Pawn;
			}
		}
	}

	return nullptr;
}
}

void SYIBagDashboard::Construct(const FArguments& InArgs)
{
	RuntimeSpellbookSlotTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Equip.Slot.Spellbook.Primary")), false);
	RuntimeSpellbookActionSlotIndex = 0;

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
				+ SVerticalBox::Slot().AutoHeight().Padding(2, 6, 2, 2)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(6)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("YOLOInventory", "BagDash_RuntimeSetupTitle", "Runtime Gameplay Setup (PIE Pawn)"))
							.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
							[
								SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "BagDash_RuntimeSlotTag", "Spellbook Slot Tag"))
							]
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SNew(SEditableTextBox)
								.Text_Lambda([this]()
								{
									return RuntimeSpellbookSlotTag.IsValid()
										? FText::FromString(RuntimeSpellbookSlotTag.ToString())
										: FText::GetEmpty();
								})
								.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
								{
									RuntimeSpellbookSlotTag = FGameplayTag::RequestGameplayTag(FName(*NewText.ToString()), false);
								})
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
							[
								SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "BagDash_RuntimeSlotIndex", "Action Slot Index"))
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SNumericEntryBox<int32>)
								.MinValue(0)
								.MaxValue(63)
								.Value_Lambda([this]() -> TOptional<int32> { return RuntimeSpellbookActionSlotIndex; })
								.OnValueChanged_Lambda([this](int32 NewValue)
								{
									RuntimeSpellbookActionSlotIndex = FMath::Clamp(NewValue, 0, 63);
								})
							]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)
							[
								SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "BagDash_RuntimeApplyPreset", "Apply Spellbook Preset"))
								.ToolTipText(NSLOCTEXT("YOLOInventory", "BagDash_RuntimeApplyPreset_TT", "Runs on the current PIE player pawn and ensures inventory/equipment/actionbar setup + spellbook autobind."))
								.OnClicked_Lambda([this]()
								{
									ApplyRuntimeSpellbookPresetFromToolbar();
									return FReply::Handled();
								})
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)
							[
								SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "BagDash_RuntimeValidate", "Validate Runtime Setup"))
								.ToolTipText(NSLOCTEXT("YOLOInventory", "BagDash_RuntimeValidate_TT", "Validates current PIE pawn inventory gameplay setup without changing components."))
								.OnClicked_Lambda([this]()
								{
									ValidateRuntimeSetupFromToolbar();
									return FReply::Handled();
								})
							]
						]
					]
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
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(4)
			[
				SAssignNew(GridHost, SBox)
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

TSharedRef<SWidget> SYIBagDashboard::GetDetailsPanelWidget() const
{
	SYIBagDashboard* Self = const_cast<SYIBagDashboard*>(this);
	if (!Self->DetailsView.IsValid())
	{
		FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailArgs;
		DetailArgs.bAllowSearch = true;
		DetailArgs.bHideSelectionTip = true;
		Self->DetailsView = PropModule.CreateDetailView(DetailArgs);
		if (Self->SelectedBag.IsValid())
		{
			Self->DetailsView->SetObject(Self->SelectedBag.Get());
		}
	}
	return Self->DetailsView.ToSharedRef();
}

void SYIBagDashboard::SaveCurrentBagFromToolbar()
{
	SaveCurrentBag();
}

void SYIBagDashboard::CreateBagFromToolbar()
{
	CreateNewBag();
}

void SYIBagDashboard::ApplyRuntimeSpellbookPresetFromToolbar()
{
	APawn* Pawn = YIBagDash_ResolveRuntimePawn();
	if (!Pawn)
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_Runtime_NoPawn", "Runtime preset failed: run PIE and possess a pawn first.");
		return;
	}

	FYIInventoryGameplaySetupResult SetupResult;
	const bool bOk = UYIInventoryGameplaySetupLibrary::ApplySpellbookActionPreset(
		Pawn,
		RuntimeSpellbookSlotTag,
		RuntimeSpellbookActionSlotIndex,
		SetupResult);

	StatusText = FText::FromString(SetupResult.Summary);
	if (!bOk && SetupResult.BlockingIssues.Num() > 0)
	{
		StatusText = FText::FromString(SetupResult.BlockingIssues[0]);
	}
}

void SYIBagDashboard::ValidateRuntimeSetupFromToolbar()
{
	APawn* Pawn = YIBagDash_ResolveRuntimePawn();
	if (!Pawn)
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_RuntimeValidate_NoPawn", "Runtime validation failed: run PIE and possess a pawn first.");
		return;
	}

	FYIInventoryGameplaySetupResult ValidateResult;
	const bool bOk = UYIInventoryGameplaySetupLibrary::ValidatePawnInventoryGameplaySetup(Pawn, ValidateResult);
	StatusText = FText::FromString(ValidateResult.Summary);
	if (!bOk && ValidateResult.BlockingIssues.Num() > 0)
	{
		StatusText = FText::FromString(ValidateResult.BlockingIssues[0]);
	}
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
