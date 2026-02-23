#include "SYIBagDashboard.h"
#include "SBagEditor.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIEquipmentSchemaAsset.h"
#include "YIBagItemDetailsProxy.h"
#include "YIInventoryComponent.h"
#include "YIEquipmentComponent.h"
#include "YIActionBarComponent.h"
#include "YIInventoryBagFactory.h"
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

static void YIBagDash_BuildRuntimeValidationSummary(
	APawn* Pawn,
	TArray<FString>& OutBlockingIssues,
	TArray<FString>& OutWarnings,
	FString& OutSummary)
{
	if (!Pawn)
	{
		OutBlockingIssues.Add(TEXT("Pawn is null."));
		OutSummary = TEXT("Setup FAILED. Blocking=1 Warnings=0");
		return;
	}

	UYIInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	UYIEquipmentComponent* EquipmentComp = Pawn->FindComponentByClass<UYIEquipmentComponent>();
	UYIActionBarComponent* ActionBarComp = Pawn->FindComponentByClass<UYIActionBarComponent>();

	if (!InventoryComp)
	{
		OutBlockingIssues.Add(FString::Printf(TEXT("Pawn '%s' is missing UYIInventoryComponent."), *Pawn->GetName()));
	}
	else
	{
		const bool bHasActiveBag = InventoryComp->GetBag() != nullptr || InventoryComp->EquippedBag != nullptr;
		if (!bHasActiveBag && InventoryComp->Bags.Num() == 0)
		{
			OutBlockingIssues.Add(FString::Printf(TEXT("Pawn '%s' has no active bag and Bags array is empty."), *Pawn->GetName()));
		}
		else if (!bHasActiveBag)
		{
			OutWarnings.Add(FString::Printf(TEXT("Pawn '%s' has Bags entries but no active opened bag."), *Pawn->GetName()));
		}
	}

	if (!EquipmentComp)
	{
		OutWarnings.Add(FString::Printf(TEXT("Pawn '%s' is missing UYIEquipmentComponent."), *Pawn->GetName()));
	}
	else
	{
		TArray<FString> EquipmentBlocking;
		TArray<FString> EquipmentWarnings;
		EquipmentComp->ValidateEquipmentSetup(EquipmentBlocking, EquipmentWarnings);
		for (const FString& Issue : EquipmentBlocking)
		{
			OutBlockingIssues.Add(FString::Printf(TEXT("Equipment setup: %s"), *Issue));
		}
		for (const FString& Warning : EquipmentWarnings)
		{
			OutWarnings.Add(FString::Printf(TEXT("Equipment setup: %s"), *Warning));
		}
	}

	if (!ActionBarComp)
	{
		OutWarnings.Add(FString::Printf(TEXT("Pawn '%s' is missing UYIActionBarComponent."), *Pawn->GetName()));
	}
	else
	{
		TArray<FString> ActionBlocking;
		TArray<FString> ActionWarnings;
		ActionBarComp->ValidateActionBindings(ActionBlocking, ActionWarnings);
		for (const FString& Issue : ActionBlocking)
		{
			OutBlockingIssues.Add(FString::Printf(TEXT("Action bar setup: %s"), *Issue));
		}
		for (const FString& Warning : ActionWarnings)
		{
			OutWarnings.Add(FString::Printf(TEXT("Action bar setup: %s"), *Warning));
		}
	}

	OutSummary = FString::Printf(
		TEXT("Setup %s. Blocking=%d Warnings=%d"),
		OutBlockingIssues.Num() == 0 ? TEXT("OK") : TEXT("FAILED"),
		OutBlockingIssues.Num(),
		OutWarnings.Num());
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
								SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "BagDash_RuntimeSlotTag", "Context Slot Tag"))
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
								.Text(NSLOCTEXT("YOLOInventory", "BagDash_RuntimeApplyPreset", "Apply Context Preset"))
								.ToolTipText(NSLOCTEXT("YOLOInventory", "BagDash_RuntimeApplyPreset_TT", "Runs on the current PIE player pawn and ensures inventory/equipment/actionbar setup + context-grid autobind."))
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
}

void SYIBagDashboard::OpenAsset(UObject* Asset)
{
	if (UYIInventoryBag* Bag = Cast<UYIInventoryBag>(Asset))
	{
		SetSelectedBag(Bag);
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
		Self->EquipmentLayoutDetailsView->SetObject(Self->SelectedEquipmentSchema.Get());
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
			+ SVerticalBox::Slot().FillHeight(0.28f).Padding(2, 0, 2, 6)
			[
				Self->BuildEquipmentSchemaPicker()
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2, 2, 2, 4)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("YOLOInventory", "BagDash_SchemaPanel_DetailsTitle", "Schema Details"))
			]
			+ SVerticalBox::Slot().FillHeight(0.72f).Padding(2, 0, 2, 0)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(4)
				[
					Self->EquipmentLayoutDetailsView.ToSharedRef()
				]
			];
	}

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
	SaveCurrentEquipmentSchema();
}

void SYIBagDashboard::ApplyRuntimeSpellbookPresetFromToolbar()
{
	APawn* Pawn = YIBagDash_ResolveRuntimePawn();
	if (!Pawn)
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_Runtime_NoPawn", "Runtime preset failed: run PIE and possess a pawn first.");
		return;
	}

	UYIInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	UYIEquipmentComponent* EquipmentComp = Pawn->FindComponentByClass<UYIEquipmentComponent>();
	UYIActionBarComponent* ActionBarComp = Pawn->FindComponentByClass<UYIActionBarComponent>();
	if (!InventoryComp || !EquipmentComp || !ActionBarComp)
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_Runtime_PresetMissingComponents", "Runtime preset failed: pawn is missing required inventory/equipment/actionbar components.");
		return;
	}

	if (!Pawn->HasAuthority())
	{
		StatusText = NSLOCTEXT("YOLOInventory", "BagDash_Runtime_PresetNoAuthority", "Runtime preset must run on authority (server or single-player PIE).");
		return;
	}

	if (InventoryComp->Bags.Num() == 0)
	{
		InventoryComp->CreateBag(TEXT("Inventory"), FIntPoint(10, 6));
	}
	if (!InventoryComp->GetBag() && InventoryComp->Bags.Num() > 0)
	{
		InventoryComp->OpenBag(InventoryComp->Bags[0]);
	}

	FYIEquipmentActionAutoBindRule Rule;
	Rule.bEnabled = true;
	Rule.EquipSlotTag = RuntimeSpellbookSlotTag;
	Rule.ActionSlotIndex = FMath::Max(0, RuntimeSpellbookActionSlotIndex);
	Rule.bAllowOverrideExistingBinding = true;
	Rule.bClearWhenUnequipped = true;

	ActionBarComp->InitializeActionSlots(FMath::Max(RuntimeSpellbookActionSlotIndex + 1, 1));
	ActionBarComp->bAutoBindFromEquipment = true;
	ActionBarComp->AutoBindRules.Reset();
	ActionBarComp->AutoBindRules.Add(Rule);
	ActionBarComp->RebuildAutoBindingsFromEquipment(EquipmentComp);

	TArray<FString> BlockingIssues;
	TArray<FString> Warnings;
	FString Summary;
	YIBagDash_BuildRuntimeValidationSummary(Pawn, BlockingIssues, Warnings, Summary);
	StatusText = FText::FromString(Summary);
	if (BlockingIssues.Num() > 0)
	{
		StatusText = FText::FromString(BlockingIssues[0]);
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

	TArray<FString> BlockingIssues;
	TArray<FString> Warnings;
	FString Summary;
	YIBagDash_BuildRuntimeValidationSummary(Pawn, BlockingIssues, Warnings, Summary);
	StatusText = FText::FromString(Summary);
	if (BlockingIssues.Num() > 0)
	{
		StatusText = FText::FromString(BlockingIssues[0]);
	}
}

void SYIBagDashboard::SetSelectedBag(UYIInventoryBag* InBag)
{
	SelectedBag = InBag;
	SelectedBagItemProxy.Reset();
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
				.OnSelectionChanged(SBagEditor::FOnSelectionChanged::CreateSP(this, &SYIBagDashboard::HandleGridSelectionChanged))
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
		SelectedBagItemProxy.Reset();
		if (DetailsView.IsValid())
		{
			DetailsView->SetObject(nullptr);
		}
	}
}

void SYIBagDashboard::HandleGridSelectionChanged(int32 SelectedIndex)
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	UYIInventoryBag* Bag = SelectedBag.Get();
	if (!Bag || !Bag->Items.IsValidIndex(SelectedIndex))
	{
		SelectedBagItemProxy.Reset();
		DetailsView->SetObject(Bag);
		return;
	}

	if (!SelectedBagItemProxy.IsValid())
	{
		SelectedBagItemProxy = TStrongObjectPtr<UYIBagItemDetailsProxy>(NewObject<UYIBagItemDetailsProxy>(GetTransientPackage()));
	}

	SelectedBagItemProxy->LoadFromBag(Bag, SelectedIndex);
	DetailsView->SetObject(SelectedBagItemProxy.Get());
}

FReply SYIBagDashboard::SaveCurrentBag()
{
	UYIInventoryBag* Bag = SelectedBag.Get();
	if (!Bag)
	{
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

void SYIBagDashboard::SetSelectedEquipmentSchema(UYIEquipmentSchemaAsset* InSchema)
{
	SelectedEquipmentSchema = InSchema;
	if (EquipmentLayoutDetailsView.IsValid())
	{
		EquipmentLayoutDetailsView->SetObject(InSchema);
	}
}

UYIEquipmentSchemaAsset* SYIBagDashboard::GetSelectedEquipmentSchema() const
{
	return SelectedEquipmentSchema.Get();
}
