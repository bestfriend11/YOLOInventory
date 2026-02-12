#include "SYIGeneratorDashboard.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemGenerator.h"
#include "YIItemDefinition.h"
#include "YIItemRegistrySubsystem.h"
#include "YIAffixAsset.h"
#include "YILootTableFactory.h"
#include "YIRarityProfileFactory.h"
#include "YIItemGeneratorFactory.h"
#include "YIAffixPoolAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#include "Misc/PackageName.h"
#include "FileHelpers.h"
#include "PropertyCustomizationHelpers.h"

static bool YIGeneratorDash_SaveObjectPackage(UObject* ObjectToSave)
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

void SYIGeneratorDashboard::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailArgs;
	DetailArgs.bAllowSearch = true;
	DetailArgs.bHideSelectionTip = true;
	DetailsView = PropModule.CreateDetailView(DetailArgs);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &SYIGeneratorDashboard::HandleDetailsChanged);
	LayoutMode = InArgs._LayoutMode;

	AssetPanelWidget = BuildAssetPicker();
	DetailsPanelWidget = BuildDetailsPanelWidget();
	TestPanelWidget = BuildTestPanelWidget();

	if (LayoutMode == EYIGeneratorDashboardLayout::AssetListOnly)
	{
		ChildSlot
		[
			AssetPanelWidget.ToSharedRef()
		];
		return;
	}

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.42f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				AssetPanelWidget.ToSharedRef()
			]
		]
		+ SSplitter::Slot().Value(0.58f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				DetailsPanelWidget.ToSharedRef()
			]
		]
	];
}

TSharedRef<SWidget> SYIGeneratorDashboard::GetAssetPanelWidget() const
{
	SYIGeneratorDashboard* Self = const_cast<SYIGeneratorDashboard*>(this);
	if (!Self->AssetPanelWidget.IsValid())
	{
		Self->AssetPanelWidget = Self->BuildAssetPicker();
	}
	return Self->AssetPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIGeneratorDashboard::GetDetailsPanelWidget() const
{
	SYIGeneratorDashboard* Self = const_cast<SYIGeneratorDashboard*>(this);
	if (!Self->DetailsPanelWidget.IsValid())
	{
		Self->DetailsPanelWidget = Self->BuildDetailsPanelWidget();
	}
	return Self->DetailsPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIGeneratorDashboard::GetTestPanelWidget() const
{
	SYIGeneratorDashboard* Self = const_cast<SYIGeneratorDashboard*>(this);
	if (!Self->TestPanelWidget.IsValid())
	{
		Self->TestPanelWidget = Self->BuildTestPanelWidget();
	}
	return Self->TestPanelWidget.ToSharedRef();
}

void SYIGeneratorDashboard::CreateLootTableFromToolbar()
{
	CreateLootTable();
}

void SYIGeneratorDashboard::CreateRarityProfileFromToolbar()
{
	CreateRarityProfile();
}

void SYIGeneratorDashboard::CreateItemGeneratorFromToolbar()
{
	CreateItemGenerator();
}

void SYIGeneratorDashboard::RunGeneratorTestFromToolbar()
{
	RunGeneratorTest();
}

void SYIGeneratorDashboard::PopulateLootTableFromDataSourcesFromToolbar()
{
	PopulateLootTableFromDataSources();
}

void SYIGeneratorDashboard::SaveCurrentAssetFromToolbar()
{
	UObject* ObjectToSave = SelectedAsset.Get();
	if (!ObjectToSave)
	{
		TestResult = NSLOCTEXT("YOLOInventory","GenDash_Save_NoSelection","Save skipped: no selected generator/loot/rarity asset.");
		return;
	}

	if (YIGeneratorDash_SaveObjectPackage(ObjectToSave))
	{
		TestResult = FText::Format(
			NSLOCTEXT("YOLOInventory","GenDash_Save_Success","Saved: {0}"),
			FText::FromString(ObjectToSave->GetPathName()));
	}
	else
	{
		TestResult = NSLOCTEXT("YOLOInventory","GenDash_Save_Failed","Save was canceled or failed.");
	}
}

void SYIGeneratorDashboard::GuidedSetupFromToolbar()
{
	UObject* Current = SelectedAsset.Get();
	if (!Current)
	{
		TestResult = NSLOCTEXT("YOLOInventory","GenDash_Guided_NoSelection","Guided setup: select a generator or loot table first.");
		return;
	}

	if (UYILootTable* LootTable = Cast<UYILootTable>(Current))
	{
		PopulateLootTableFromDataSources();
		if (LootTable->Entries.Num() > 0)
		{
			TestResult = FText::Format(
				NSLOCTEXT("YOLOInventory","GenDash_Guided_LootOnly","Guided setup complete. Loot table now has {0} entries."),
				FText::AsNumber(LootTable->Entries.Num()));
		}
		return;
	}

	UYIItemGenerator* Generator = Cast<UYIItemGenerator>(Current);
	if (!Generator)
	{
		TestResult = NSLOCTEXT("YOLOInventory","GenDash_Guided_SelectGenOrLoot","Guided setup works with ItemGenerator or LootTable selections.");
		return;
	}

	UYILootTable* LootTable = Generator->LootTable.IsValid() ? Generator->LootTable.Get() : Generator->LootTable.LoadSynchronous();
	if (!LootTable)
	{
		TestResult = NSLOCTEXT("YOLOInventory","GenDash_Guided_MissingLoot","Guided setup: selected generator has no loot table assigned.");
		return;
	}

	PopulateLootTableFromDataSources();
	const bool bTestRan = RunGeneratorTestInternal(false);
	if (!bTestRan)
	{
		TestResult = FText::Format(
			NSLOCTEXT("YOLOInventory","GenDash_Guided_PopulatedNoTest","Guided setup populated loot table ({0} entries), but test roll failed."),
			FText::AsNumber(LootTable->Entries.Num()));
		return;
	}
}

UYIItemGenerator* SYIGeneratorDashboard::GetSelectedGenerator() const
{
	return Cast<UYIItemGenerator>(SelectedAsset.Get());
}

void SYIGeneratorDashboard::MarkGeneratorDirty()
{
	if (UYIItemGenerator* Generator = GetSelectedGenerator())
	{
		Generator->Modify();
		if (UPackage* Package = Generator->GetOutermost())
		{
			Package->MarkPackageDirty();
		}
	}
}

void SYIGeneratorDashboard::ApplyGeneratorPreset(int32 PresetId)
{
	UYIItemGenerator* Generator = GetSelectedGenerator();
	if (!Generator)
	{
		TestResult = NSLOCTEXT("YOLOInventory","GenDash_Preset_NoGenerator","Select an Item Generator before applying presets.");
		return;
	}

	Generator->Modify();
	if (PresetId == 0) // Casual baseline
	{
		Generator->bApplyTemplateAffixes = true;
		Generator->bGenerateRandomAffixes = true;
		Generator->bUseDefinitionAffixPools = true;
		Generator->bClampLevel = true;
		Generator->MinItemLevel = 1;
		Generator->MaxItemLevel = 60;
		Generator->PrefixCriteria.bEnabled = false;
		Generator->SuffixCriteria.bEnabled = false;
	}
	else if (PresetId == 1) // DS-style baseline
	{
		Generator->bApplyTemplateAffixes = true;
		Generator->bGenerateRandomAffixes = true;
		Generator->bUseDefinitionAffixPools = true;
		Generator->bClampLevel = true;
		Generator->MinItemLevel = 1;
		Generator->MaxItemLevel = 100;

		Generator->PrefixCriteria.bEnabled = true;
		Generator->PrefixCriteria.bUseItemLevelAsPowerBaseline = true;
		Generator->PrefixCriteria.MinTier = 1;
		Generator->PrefixCriteria.MaxTier = 9999;
		Generator->PrefixCriteria.MinPowerLevelOffset = -2;
		Generator->PrefixCriteria.MaxPowerLevelOffset = 5;

		Generator->SuffixCriteria.bEnabled = true;
		Generator->SuffixCriteria.bUseItemLevelAsPowerBaseline = true;
		Generator->SuffixCriteria.MinTier = 1;
		Generator->SuffixCriteria.MaxTier = 9999;
		Generator->SuffixCriteria.MinPowerLevelOffset = -2;
		Generator->SuffixCriteria.MaxPowerLevelOffset = 5;
	}
	else // No random affixes
	{
		Generator->bApplyTemplateAffixes = true;
		Generator->bGenerateRandomAffixes = false;
		Generator->PrefixCriteria.bEnabled = false;
		Generator->SuffixCriteria.bEnabled = false;
	}

	MarkGeneratorDirty();
	TestResult = NSLOCTEXT("YOLOInventory","GenDash_PresetApplied","Preset applied.");
	bPendingAutoRefresh = bAutoRefreshTest;
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

TSharedRef<SWidget> SYIGeneratorDashboard::BuildDetailsPanelWidget()
{
	return DetailsView.IsValid()
		? StaticCastSharedRef<SWidget>(DetailsView.ToSharedRef())
		: StaticCastSharedRef<SWidget>(SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "GenDash_NoDetails", "Details panel unavailable")));
}

TSharedRef<SWidget> SYIGeneratorDashboard::BuildTestPanelWidget()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6, 2)
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(8)
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
						[
							SNew(STextBlock)
								.Text(NSLOCTEXT("YOLOInventory","GenDash_DesignerTitle","Designer Quick Setup"))
								.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory","GenDash_Preset_Casual","Preset: Casual"))
										.OnClicked_Lambda([this]()
											{
												ApplyGeneratorPreset(0);
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory","GenDash_Preset_DS","Preset: DS-style"))
										.OnClicked_Lambda([this]()
											{
												ApplyGeneratorPreset(1);
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory","GenDash_Preset_NoAffix","Preset: No Random Affixes"))
										.OnClicked_Lambda([this]()
											{
												ApplyGeneratorPreset(2);
												return FReply::Handled();
											})
								]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_QS_Loot","Loot Table"))
								]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SObjectPropertyEntryBox)
										.AllowedClass(UYILootTable::StaticClass())
										.ObjectPath_Lambda([this]()
											{
												if (const UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													return Generator->LootTable.ToSoftObjectPath().ToString();
												}
												return FString();
											})
										.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->LootTable = TSoftObjectPtr<UYILootTable>(AssetData.ToSoftObjectPath());
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
											})
								]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_QS_Rarity","Rarity Profile"))
								]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SObjectPropertyEntryBox)
										.AllowedClass(UYIRarityProfile::StaticClass())
										.ObjectPath_Lambda([this]()
											{
												if (const UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													return Generator->RarityProfile.ToSoftObjectPath().ToString();
												}
												return FString();
											})
										.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->RarityProfile = TSoftObjectPtr<UYIRarityProfile>(AssetData.ToSoftObjectPath());
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
											})
								]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0).VAlign(VAlign_Center)
								[
									SNew(SCheckBox)
										.IsChecked_Lambda([this]()
											{
												const UYIItemGenerator* Generator = GetSelectedGenerator();
												return (Generator && Generator->bGenerateRandomAffixes) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
											})
										.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->bGenerateRandomAffixes = (NewState == ECheckBoxState::Checked);
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
											})
										[
											SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_QS_RandomAffix","Random Affixes"))
										]
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0).VAlign(VAlign_Center)
								[
									SNew(SCheckBox)
										.IsChecked_Lambda([this]()
											{
												const UYIItemGenerator* Generator = GetSelectedGenerator();
												return (Generator && Generator->bUseDefinitionAffixPools) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
											})
										.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->bUseDefinitionAffixPools = (NewState == ECheckBoxState::Checked);
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
											})
										[
											SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_QS_DefPools","Use Item Default Pools"))
										]
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(SCheckBox)
										.IsChecked_Lambda([this]()
											{
												const UYIItemGenerator* Generator = GetSelectedGenerator();
												return (Generator && Generator->bClampLevel) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
											})
										.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->bClampLevel = (NewState == ECheckBoxState::Checked);
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
											})
										[
											SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_QS_Clamp","Clamp Level"))
										]
								]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_QS_LevelWindow","Generator Level Window"))
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
								[
									SNew(SNumericEntryBox<int32>)
										.MinValue(1)
										.MaxValue(9999)
										.Value_Lambda([this]() -> TOptional<int32>
											{
												if (const UYIItemGenerator* Generator = GetSelectedGenerator()) { return Generator->MinItemLevel; }
												return 1;
											})
										.OnValueChanged_Lambda([this](int32 V)
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->MinItemLevel = FMath::Clamp(V, 1, 9999);
													if (Generator->MaxItemLevel < Generator->MinItemLevel)
													{
														Generator->MaxItemLevel = Generator->MinItemLevel;
													}
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_QS_To","to"))
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
								[
									SNew(SNumericEntryBox<int32>)
										.MinValue(1)
										.MaxValue(9999)
										.Value_Lambda([this]() -> TOptional<int32>
											{
												if (const UYIItemGenerator* Generator = GetSelectedGenerator()) { return Generator->MaxItemLevel; }
												return 100;
											})
										.OnValueChanged_Lambda([this](int32 V)
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->MaxItemLevel = FMath::Clamp(V, 1, 9999);
													if (Generator->MinItemLevel > Generator->MaxItemLevel)
													{
														Generator->MinItemLevel = Generator->MaxItemLevel;
													}
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
											})
								]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_QS_AffixCriteria","Affix Criteria"))
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory","GenDash_QS_CriteriaRelaxed","Relaxed"))
										.OnClicked_Lambda([this]()
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->PrefixCriteria.bEnabled = true;
													Generator->SuffixCriteria.bEnabled = true;
													Generator->PrefixCriteria.MinPowerLevelOffset = -5;
													Generator->PrefixCriteria.MaxPowerLevelOffset = 10;
													Generator->SuffixCriteria.MinPowerLevelOffset = -5;
													Generator->SuffixCriteria.MaxPowerLevelOffset = 10;
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory","GenDash_QS_CriteriaBalanced","Balanced"))
										.OnClicked_Lambda([this]()
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->PrefixCriteria.bEnabled = true;
													Generator->SuffixCriteria.bEnabled = true;
													Generator->PrefixCriteria.MinPowerLevelOffset = -2;
													Generator->PrefixCriteria.MaxPowerLevelOffset = 5;
													Generator->SuffixCriteria.MinPowerLevelOffset = -2;
													Generator->SuffixCriteria.MaxPowerLevelOffset = 5;
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory","GenDash_QS_CriteriaStrict","Strict"))
										.OnClicked_Lambda([this]()
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->PrefixCriteria.bEnabled = true;
													Generator->SuffixCriteria.bEnabled = true;
													Generator->PrefixCriteria.MinPowerLevelOffset = 0;
													Generator->PrefixCriteria.MaxPowerLevelOffset = 2;
													Generator->SuffixCriteria.MinPowerLevelOffset = 0;
													Generator->SuffixCriteria.MaxPowerLevelOffset = 2;
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory","GenDash_QS_CriteriaOff","Disable"))
										.OnClicked_Lambda([this]()
											{
												if (UYIItemGenerator* Generator = GetSelectedGenerator())
												{
													Generator->Modify();
													Generator->PrefixCriteria.bEnabled = false;
													Generator->SuffixCriteria.bEnabled = false;
													MarkGeneratorDirty();
													bPendingAutoRefresh = bAutoRefreshTest;
												}
												return FReply::Handled();
											})
								]
						]
				]
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
						.OnValueChanged_Lambda([this](int32 V){ TestLevel = V; bPendingAutoRefresh = bAutoRefreshTest; })
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
						.OnValueChanged_Lambda([this](int32 V){ TestSeed = V; bPendingAutoRefresh = bAutoRefreshTest; })
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
						.OnValueChanged_Lambda([this](int32 V){ TestRuns = FMath::Clamp(V, 1, 200); bPendingAutoRefresh = bAutoRefreshTest; })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8,2,2,2)
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bAutoRefreshTest ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
							{
								bAutoRefreshTest = (NewState == ECheckBoxState::Checked);
								bPendingAutoRefresh = bAutoRefreshTest;
								NextAutoRefreshTime = 0.0;
							})
						[
							SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","GenDash_AutoRefresh","Auto"))
						]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8,2,2,2)
				[
					SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory","GenDash_PopulateLoot","Populate Loot From DataSource Items"))
						.OnClicked(this, &SYIGeneratorDashboard::PopulateLootTableFromDataSources)
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
		];
}

void SYIGeneratorDashboard::OnAssetSelected(const FAssetData& AssetData)
{
	SelectedAsset = AssetData.GetAsset();
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(SelectedAsset.Get());
	}
	bPendingAutoRefresh = bAutoRefreshTest;
	NextAutoRefreshTime = 0.0;
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
	RunGeneratorTestInternal(true);
	return FReply::Handled();
}

bool SYIGeneratorDashboard::RunGeneratorTestInternal(bool bUserInitiated)
{
	TestResult = FText::GetEmpty();
	UYIItemGenerator* Generator = Cast<UYIItemGenerator>(SelectedAsset.Get());
	if (!Generator)
	{
		if (bUserInitiated)
		{
			TestResult = NSLOCTEXT("YOLOInventory","GenDash_SelectGenerator","Select an Item Generator asset first.");
		}
		return false;
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
			if (bUserInitiated)
			{
				TestResult = NSLOCTEXT("YOLOInventory","GenDash_GenerateFailed","Generator failed to roll an item. Check loot table and definitions.");
			}
			return false;
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
			if (bUserInitiated)
			{
				TestResult = NSLOCTEXT("YOLOInventory","GenDash_GenerateFailed","Generator failed to roll an item. Check loot table and definitions.");
			}
			return false;
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
	return true;
}

FReply SYIGeneratorDashboard::PopulateLootTableFromDataSources()
{
	UYILootTable* LootTable = Cast<UYILootTable>(SelectedAsset.Get());
	if (!LootTable)
	{
		if (UYIItemGenerator* Generator = Cast<UYIItemGenerator>(SelectedAsset.Get()))
		{
			LootTable = Generator->LootTable.IsValid() ? Generator->LootTable.Get() : Generator->LootTable.LoadSynchronous();
		}
	}
	if (!LootTable)
	{
		TestResult = NSLOCTEXT("YOLOInventory","GenDash_SelectLootTable","Select a Loot Table (or Item Generator with LootTable assigned) first.");
		return FReply::Handled();
	}

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> ItemAssets;
	AssetRegistry.Get().GetAssetsByClass(UYIItemDefinition::StaticClass()->GetClassPathName(), ItemAssets, true);

	TMap<FSoftObjectPath, FYILootTableEntry> ExistingByDefPath;
	for (const FYILootTableEntry& Entry : LootTable->Entries)
	{
		if (Entry.Definition.ToSoftObjectPath().IsValid())
		{
			ExistingByDefPath.FindOrAdd(Entry.Definition.ToSoftObjectPath()) = Entry;
		}
	}

	TMap<int64, TSoftObjectPtr<UYIItemDefinition>> AssetByCode;
	TArray<TSoftObjectPtr<UYIItemDefinition>> DataSourceBackedDefs;
	DataSourceBackedDefs.Reserve(ItemAssets.Num());
	for (const FAssetData& AssetData : ItemAssets)
	{
		UYIItemDefinition* Def = Cast<UYIItemDefinition>(AssetData.GetAsset());
		if (!Def)
		{
			continue;
		}
		if (Def->UniqueCode != 0)
		{
			AssetByCode.FindOrAdd(Def->UniqueCode) = Def;
		}
		if (!Def->bGeneratedFromDataSource && !Def->SourceDataSource.ToSoftObjectPath().IsValid())
		{
			continue;
		}
		DataSourceBackedDefs.Add(Def);
	}

	// Preferred path: datasource rows from registry that have generated assets by code.
	if (GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			TArray<FYIItemRegistryView> Views;
			Registry->GetAllItems(Views, true);
			for (const FYIItemRegistryView& View : Views)
			{
				if (!View.bIsDataTable || View.UniqueCode == 0)
				{
					continue;
				}
				if (const TSoftObjectPtr<UYIItemDefinition>* Found = AssetByCode.Find(View.UniqueCode))
				{
					DataSourceBackedDefs.AddUnique(*Found);
				}
			}
		}
	}

	DataSourceBackedDefs.Sort([](const TSoftObjectPtr<UYIItemDefinition>& A, const TSoftObjectPtr<UYIItemDefinition>& B)
	{
		const UYIItemDefinition* DefA = A.Get();
		const UYIItemDefinition* DefB = B.Get();
		if (!DefA || !DefB)
		{
			return A.ToSoftObjectPath().ToString() < B.ToSoftObjectPath().ToString();
		}
		if (DefA->UniqueCode == DefB->UniqueCode)
		{
			return DefA->GetName() < DefB->GetName();
		}
		return DefA->UniqueCode < DefB->UniqueCode;
	});

	TArray<FYILootTableEntry> NewEntries;
	NewEntries.Reserve(DataSourceBackedDefs.Num());
	for (const TSoftObjectPtr<UYIItemDefinition>& DefSoft : DataSourceBackedDefs)
	{
		if (!DefSoft.ToSoftObjectPath().IsValid())
		{
			continue;
		}

		const FSoftObjectPath DefPath = DefSoft.ToSoftObjectPath();
		if (const FYILootTableEntry* Existing = ExistingByDefPath.Find(DefPath))
		{
			NewEntries.Add(*Existing);
			continue;
		}

		FYILootTableEntry NewEntry;
		NewEntry.Definition = DefSoft;
		NewEntry.Weight = 1.f;
		NewEntry.MinCount = 1;
		NewEntry.MaxCount = 1;
		NewEntries.Add(NewEntry);
	}

	if (NewEntries.Num() == 0)
	{
		TestResult = NSLOCTEXT("YOLOInventory","GenDash_PopulateLootEmpty","No datasource-backed item assets found. Loot table was not modified.");
		return FReply::Handled();
	}

	LootTable->Modify();
	LootTable->Entries = MoveTemp(NewEntries);
	LootTable->MarkPackageDirty();
	TestResult = FText::Format(
		NSLOCTEXT("YOLOInventory","GenDash_PopulateLootResult","Loot table synced from datasource-backed items. Entries: {0}"),
		FText::AsNumber(LootTable->Entries.Num()));
	return FReply::Handled();
}

void SYIGeneratorDashboard::HandleDetailsChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	(void)PropertyChangedEvent;
	bPendingAutoRefresh = bAutoRefreshTest;
	NextAutoRefreshTime = 0.0;
}

void SYIGeneratorDashboard::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (!bAutoRefreshTest || !bPendingAutoRefresh)
	{
		return;
	}
	if (InCurrentTime < NextAutoRefreshTime)
	{
		return;
	}

	RunGeneratorTestInternal(false);
	bPendingAutoRefresh = false;
	NextAutoRefreshTime = InCurrentTime + 0.2;
}
