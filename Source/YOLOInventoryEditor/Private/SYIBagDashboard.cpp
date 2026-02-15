#include "SYIBagDashboard.h"
#include "SBagEditor.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIEquipmentLayoutAsset.h"
#include "YIEquipmentSchemaAsset.h"
#include "YIInventoryGameplaySetupLibrary.h"
#include "YIInventoryBagFactory.h"
#include "YIEquipmentLayoutAssetFactory.h"
#include "YIEquipmentSchemaAssetFactory.h"
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
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SBoxPanel.h"
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
#include "Algo/Sort.h"

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
	const bool bPromptToSave = true;
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
	EquipmentLayoutDetailsView = PropModule.CreateDetailView(DetailArgs);
	if (EquipmentLayoutDetailsView.IsValid())
	{
		EquipmentLayoutDetailsView->OnFinishedChangingProperties().AddLambda([this](const FPropertyChangedEvent&)
		{
			RebuildEquipmentLayoutPreview();
		});
	}

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
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "BagDash_NewLayout", "New Equip Layout"))
						.OnClicked(this, &SYIBagDashboard::CreateNewEquipmentLayout)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "BagDash_SaveLayout", "Save Equip Layout"))
						.OnClicked(this, &SYIBagDashboard::SaveCurrentEquipmentLayout)
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
					+ SSplitter::Slot().Value(0.34f)
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
					+ SSplitter::Slot().Value(0.66f)
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
			.Clipping(EWidgetClipping::ClipToBoundsAlways)
			[
				SAssignNew(GridHost, SBox)
				.Clipping(EWidgetClipping::ClipToBoundsAlways)
			]
		]
	];

	RebuildBagView();
	RebuildEquipmentLayoutPreview();
}

void SYIBagDashboard::OpenAsset(UObject* Asset)
{
	if (UYIInventoryBag* Bag = Cast<UYIInventoryBag>(Asset))
	{
		SetSelectedBag(Bag);
		return;
	}

	if (UYIEquipmentLayoutAsset* Layout = Cast<UYIEquipmentLayoutAsset>(Asset))
	{
		SetSelectedEquipmentLayout(Layout);
		return;
	}

	if (UYIEquipmentSchemaAsset* Schema = Cast<UYIEquipmentSchemaAsset>(Asset))
	{
		SetSelectedEquipmentSchema(Schema);
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

TSharedRef<SWidget> SYIBagDashboard::GetEquipmentLayoutPanelWidget() const
{
	SYIBagDashboard* Self = const_cast<SYIBagDashboard*>(this);
	if (!Self->EquipmentLayoutDetailsView.IsValid())
	{
		FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailArgs;
		DetailArgs.bAllowSearch = true;
		DetailArgs.bHideSelectionTip = true;
		Self->EquipmentLayoutDetailsView = PropModule.CreateDetailView(DetailArgs);
		if (Self->SelectedEquipmentLayout.IsValid())
		{
			Self->EquipmentLayoutDetailsView->SetObject(Self->SelectedEquipmentLayout.Get());
		}
	}

	if (!Self->EquipmentLayoutPanelWidget.IsValid())
	{
		Self->EquipmentLayoutPanelWidget =
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "BagDash_LayoutPanel_New", "New Layout"))
					.OnClicked(Self, &SYIBagDashboard::CreateNewEquipmentLayout)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "BagDash_LayoutPanel_Save", "Save Layout"))
					.OnClicked(Self, &SYIBagDashboard::SaveCurrentEquipmentLayout)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "BagDash_LayoutPanel_Refresh", "Refresh Preview"))
					.OnClicked_Lambda([Self]()
					{
						Self->RebuildEquipmentLayoutPreview();
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "BagDash_SchemaPanel_New", "New Schema"))
					.OnClicked(Self, &SYIBagDashboard::CreateNewEquipmentSchema)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "BagDash_SchemaPanel_Save", "Save Schema"))
					.OnClicked(Self, &SYIBagDashboard::SaveCurrentEquipmentSchema)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2, 2, 2, 4)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("YOLOInventory", "BagDash_SchemaPanel_ListTitle", "Equipment Schema Assets"))
			]
			+ SVerticalBox::Slot().FillHeight(0.20f).Padding(2, 0, 2, 6)
			[
				Self->BuildEquipmentSchemaPicker()
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2, 2, 2, 4)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("YOLOInventory", "BagDash_LayoutPanel_ListTitle", "Equipment Layout Assets"))
			]
			+ SVerticalBox::Slot().FillHeight(0.22f).Padding(2, 0, 2, 6)
			[
				Self->BuildEquipmentLayoutPicker()
			]
			+ SVerticalBox::Slot().FillHeight(0.58f).Padding(2, 0, 2, 0)
			[
				SNew(SSplitter)
				.Orientation(Orient_Horizontal)
				+ SSplitter::Slot().Value(0.52f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					.Padding(4)
					[
						Self->EquipmentLayoutDetailsView.ToSharedRef()
					]
				]
				+ SSplitter::Slot().Value(0.48f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(4)
					[
						SAssignNew(Self->EquipmentLayoutDockPreviewHost, SBox)
					]
				]
			];
	}

	Self->RebuildEquipmentLayoutPreview();
	return Self->EquipmentLayoutPanelWidget.ToSharedRef();
}

void SYIBagDashboard::SaveCurrentBagFromToolbar()
{
	SaveCurrentBag();
}

void SYIBagDashboard::CreateBagFromToolbar()
{
	CreateNewBag();
}

void SYIBagDashboard::SaveCurrentEquipmentLayoutFromToolbar()
{
	if (SelectedEquipmentSchema.IsValid())
	{
		SaveCurrentEquipmentSchema();
		return;
	}
	SaveCurrentEquipmentLayout();
}

void SYIBagDashboard::CreateEquipmentLayoutFromToolbar()
{
	CreateNewEquipmentLayout();
}

void SYIBagDashboard::RefreshEquipmentLayoutPreviewFromToolbar()
{
	RebuildEquipmentLayoutPreview();
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

TSharedRef<SWidget> SYIBagDashboard::BuildEquipmentLayoutPicker()
{
	FAssetPickerConfig Picker;
	Picker.InitialAssetViewType = EAssetViewType::Tile;
	Picker.Filter.ClassPaths.Add(UYIEquipmentLayoutAsset::StaticClass()->GetClassPathName());
	Picker.bAllowNullSelection = false;
	Picker.OnAssetSelected = FOnAssetSelected::CreateLambda([this](const FAssetData& AssetData)
	{
		SetSelectedEquipmentLayout(Cast<UYIEquipmentLayoutAsset>(AssetData.GetAsset()));
	});
	Picker.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([this](const FAssetData& AssetData)
	{
		SetSelectedEquipmentLayout(Cast<UYIEquipmentLayoutAsset>(AssetData.GetAsset()));
	});

	FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	return CB.Get().CreateAssetPicker(Picker);
}

TSharedRef<SWidget> SYIBagDashboard::BuildEquipmentSchemaPicker()
{
	FAssetPickerConfig Picker;
	Picker.InitialAssetViewType = EAssetViewType::Tile;
	Picker.Filter.ClassPaths.Add(UYIEquipmentSchemaAsset::StaticClass()->GetClassPathName());
	Picker.bAllowNullSelection = false;
	Picker.OnAssetSelected = FOnAssetSelected::CreateLambda([this](const FAssetData& AssetData)
	{
		SetSelectedEquipmentSchema(Cast<UYIEquipmentSchemaAsset>(AssetData.GetAsset()));
	});
	Picker.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([this](const FAssetData& AssetData)
	{
		SetSelectedEquipmentSchema(Cast<UYIEquipmentSchemaAsset>(AssetData.GetAsset()));
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
			.Clipping(EWidgetClipping::ClipToBoundsAlways)
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

void SYIBagDashboard::RebuildEquipmentLayoutPreview()
{
	if (!EquipmentLayoutPreviewHost.IsValid() && !EquipmentLayoutDockPreviewHost.IsValid())
	{
		return;
	}

	auto BuildEmptyWidget = []() -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(8)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("YOLOInventory", "BagDash_LayoutPreviewEmpty", "Select an Equipment Schema (rules) or Layout (optional preview)."))
			];
	};

	UYIEquipmentLayoutAsset* Layout = SelectedEquipmentLayout.Get();
	if (!Layout)
	{
		if (EquipmentLayoutPreviewHost.IsValid())
		{
			EquipmentLayoutPreviewHost->SetContent(BuildEmptyWidget());
		}
		if (EquipmentLayoutDockPreviewHost.IsValid())
		{
			EquipmentLayoutDockPreviewHost->SetContent(BuildEmptyWidget());
		}
		return;
	}

	TArray<FYIEquipmentSlotLayoutEntry> SortedSlots;
	Layout->GetSortedSlots(SortedSlots);
	auto BuildPreviewContainer = [&SortedSlots, Layout]() -> TSharedRef<SWidget>
	{
		const int32 AutoColumns = FMath::Max(1, Layout->AutoColumnCount);
		int32 AutoPlacementIndex = 0;
		TSharedPtr<SWidget> PreviewBody;

		if (Layout->LayoutMode == EYIEquipmentLayoutMode::Canvas)
		{
			TSharedRef<SConstraintCanvas> PreviewCanvas = SNew(SConstraintCanvas);
			for (const FYIEquipmentSlotLayoutEntry& Entry : SortedSlots)
			{
				if (!Entry.SlotTag.IsValid())
				{
					continue;
				}

				int32 Row = Entry.Row;
				int32 Column = Entry.Column;
				if (Row < 0 || Column < 0)
				{
					Row = AutoPlacementIndex / AutoColumns;
					Column = AutoPlacementIndex % AutoColumns;
					++AutoPlacementIndex;
				}

				const FVector2D AutoPos(
					(float)(Column * 98) + Layout->SlotPadding,
					(float)(Row * 98) + Layout->SlotPadding);
				const FVector2D Position = Entry.bUseCanvasPosition ? Entry.CanvasPosition : AutoPos;
				const FVector2D Size(
					FMath::Max(8.f, Entry.CanvasSize.X),
					FMath::Max(8.f, Entry.CanvasSize.Y));
				const FText SlotTitle = Entry.DisplayName.IsEmpty() ? FText::FromString(Entry.SlotTag.ToString()) : Entry.DisplayName;

				PreviewCanvas->AddSlot()
					.Offset(FMargin(Position.X, Position.Y, Size.X, Size.Y))
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						.Padding(4)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(SlotTitle)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(Entry.SlotTag.ToString()))
								.ColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f)))
							]
						]
					];
			}

			PreviewBody = SNew(SBox)
				.WidthOverride(Layout->CanvasSize.X)
				.HeightOverride(Layout->CanvasSize.Y)
				[
					PreviewCanvas
				];
		}
		else
		{
			TSharedRef<SGridPanel> PreviewGrid = SNew(SGridPanel);
			for (const FYIEquipmentSlotLayoutEntry& Entry : SortedSlots)
			{
				if (!Entry.SlotTag.IsValid())
				{
					continue;
				}

				int32 Row = Entry.Row;
				int32 Column = Entry.Column;
				if (Row < 0 || Column < 0)
				{
					Row = AutoPlacementIndex / AutoColumns;
					Column = AutoPlacementIndex % AutoColumns;
					++AutoPlacementIndex;
				}

				const FText SlotTitle = Entry.DisplayName.IsEmpty() ? FText::FromString(Entry.SlotTag.ToString()) : Entry.DisplayName;
				PreviewGrid->AddSlot(Column, Row)
					.ColumnSpan(FMath::Max(1, Entry.ColumnSpan))
					.RowSpan(FMath::Max(1, Entry.RowSpan))
					.Padding(FMargin(Layout->SlotPadding))
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						.Padding(4)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(SlotTitle)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(Entry.SlotTag.ToString()))
								.ColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f)))
							]
						]
					];
			}
			PreviewBody = PreviewGrid;
		}

		return SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2, 0, 2, 4)
				[
					SNew(STextBlock)
					.Text(FText::Format(
						NSLOCTEXT("YOLOInventory", "BagDash_LayoutPreviewTitle", "Equipment Slot Pane Preview ({0} slots) [{1}]"),
						FText::AsNumber(SortedSlots.Num()),
						Layout->LayoutMode == EYIEquipmentLayoutMode::Canvas
							? NSLOCTEXT("YOLOInventory", "BagDash_LayoutPreviewModeCanvas", "Canvas")
							: NSLOCTEXT("YOLOInventory", "BagDash_LayoutPreviewModeGrid", "Grid")))
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					PreviewBody.ToSharedRef()
				]
			];
	};

	if (EquipmentLayoutPreviewHost.IsValid())
	{
		EquipmentLayoutPreviewHost->SetContent(BuildPreviewContainer());
	}
	if (EquipmentLayoutDockPreviewHost.IsValid())
	{
		EquipmentLayoutDockPreviewHost->SetContent(BuildPreviewContainer());
	}
}

FReply SYIBagDashboard::SaveCurrentBag()
{
	UYIInventoryBag* Bag = SelectedBag.Get();
	if (!Bag)
	{
		if (SelectedEquipmentLayout.IsValid())
		{
			return SaveCurrentEquipmentLayout();
		}
		if (SelectedEquipmentSchema.IsValid())
		{
			return SaveCurrentEquipmentSchema();
		}

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

FReply SYIBagDashboard::SaveCurrentEquipmentLayout()
{
	UYIEquipmentLayoutAsset* Layout = SelectedEquipmentLayout.Get();
	if (!Layout)
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_StatusNoLayout", "No equipment layout selected.");
		return FReply::Handled();
	}

	if (YIBagDash_SaveObjectPackage(Layout))
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_StatusLayoutSaved", "Equipment layout saved.");
	}
	else
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_StatusLayoutSaveFail", "Equipment layout save canceled or failed.");
	}
	return FReply::Handled();
}

FReply SYIBagDashboard::CreateNewEquipmentLayout()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UYIEquipmentLayoutAssetFactory* Factory = NewObject<UYIEquipmentLayoutAssetFactory>();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Layouts");
	const FString BaseName = TEXT("EquipmentLayout");
	FString PackageName;
	FString AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	UObject* NewAsset = Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
	SetSelectedEquipmentLayout(Cast<UYIEquipmentLayoutAsset>(NewAsset));
	StatusText = NewAsset
		? NSLOCTEXT("YOLOInventory", "BagDash_StatusLayoutCreated", "New equipment layout created.")
		: NSLOCTEXT("YOLOInventory", "BagDash_StatusLayoutCreateFail", "Failed to create equipment layout.");
	return FReply::Handled();
}

FReply SYIBagDashboard::SaveCurrentEquipmentSchema()
{
	UYIEquipmentSchemaAsset* Schema = SelectedEquipmentSchema.Get();
	if (!Schema)
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_StatusNoSchema", "No equipment schema selected.");
		return FReply::Handled();
	}

	if (YIBagDash_SaveObjectPackage(Schema))
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_StatusSchemaSaved", "Equipment schema saved.");
	}
	else
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_StatusSchemaSaveFail", "Equipment schema save canceled or failed.");
	}
	return FReply::Handled();
}

FReply SYIBagDashboard::CreateNewEquipmentSchema()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UYIEquipmentSchemaAssetFactory* Factory = NewObject<UYIEquipmentSchemaAssetFactory>();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Schemas");
	const FString BaseName = TEXT("EquipmentSchema");
	FString PackageName;
	FString AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	UObject* NewAsset = Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
	SetSelectedEquipmentSchema(Cast<UYIEquipmentSchemaAsset>(NewAsset));
	StatusText = NewAsset
		? NSLOCTEXT("YOLOInventory", "BagDash_StatusSchemaCreated", "New equipment schema created.")
		: NSLOCTEXT("YOLOInventory", "BagDash_StatusSchemaCreateFail", "Failed to create equipment schema.");
	return FReply::Handled();
}

void SYIBagDashboard::SetSelectedEquipmentLayout(UYIEquipmentLayoutAsset* InLayout)
{
	SelectedEquipmentLayout = InLayout;
	if (InLayout)
	{
		SelectedEquipmentSchema.Reset();
	}

	if (EquipmentLayoutDetailsView.IsValid())
	{
		EquipmentLayoutDetailsView->SetObject(InLayout);
	}

	RebuildEquipmentLayoutPreview();
}

UYIEquipmentLayoutAsset* SYIBagDashboard::GetSelectedEquipmentLayout() const
{
	return SelectedEquipmentLayout.Get();
}

void SYIBagDashboard::SetSelectedEquipmentSchema(UYIEquipmentSchemaAsset* InSchema)
{
	SelectedEquipmentSchema = InSchema;
	if (InSchema)
	{
		SelectedEquipmentLayout.Reset();
	}

	if (EquipmentLayoutDetailsView.IsValid())
	{
		EquipmentLayoutDetailsView->SetObject(InSchema);
	}

	RebuildEquipmentLayoutPreview();
}

UYIEquipmentSchemaAsset* SYIBagDashboard::GetSelectedEquipmentSchema() const
{
	return SelectedEquipmentSchema.Get();
}
