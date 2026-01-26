#include "YIItemDefinitionEditor.h"
#include "YIItemDefinition.h"
// #include "YICapabilities.h" // capabilities removed
#include "YIAttributeModAsset.h"
#include "YIScriptGraph.h"
#include "YIEvolutionPath.h"
#include "YIItemVariant.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YIInventoryBag.h"
#include "PropertyEditorModule.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SHyperlink.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SScrollBox.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "SAssetDropTarget.h"
#include "Widgets/Input/SSpinBox.h"
static const FName TabID_Main("YOLOInv_ItemDef_Main");

void FYIItemDefinitionEditor::Init(UYIItemDefinition* InAsset, const TSharedPtr<IToolkitHost>& EditWithinLevelEditor)
{
	EditingAsset = InAsset;

	FPropertyEditorModule& P = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args; Args.bAllowSearch = true; Args.bHideSelectionTip = true; Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	MainDetails = P.CreateDetailView(Args);
	MainDetails->SetObject(EditingAsset);

	// Auto-refresh preview when any property changes on the item
	MainDetails->OnFinishedChangingProperties().AddLambda([this](const FPropertyChangedEvent&){ RefreshAttributeModsList(); });

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("YOLOInv_ItemDef_Layout_v3")
		->AddArea(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split(
				FTabManager::NewStack()->AddTab(TabID_Main, ETabState::OpenedTab)->SetHideTabWell(true)
			)
		);

	const TSharedRef<FWorkspaceItem> AppMenuGroup = FWorkspaceItem::NewGroup(NSLOCTEXT("YOLOInventory","AppMenu","YOLOInventory"));

	FAssetEditorToolkit::InitAssetEditor(EToolkitMode::Standalone, EditWithinLevelEditor, FName("YIItemDefEditorApp"), Layout, true, true, InAsset);
	AddMenuExtender(MakeShareable(new FExtender));
	RegenerateMenusAndToolbars();

	// Register single main tab spawner
	TabManager->RegisterTabSpawner(TabID_Main, FOnSpawnTab::CreateRaw(this, &FYIItemDefinitionEditor::SpawnMainTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory","MainTab","Item Definition"));

	// Ensure the main tab is actually invoked and visible on open
	TabManager->TryInvokeTab(TabID_Main);
}

TSharedRef<SDockTab> FYIItemDefinitionEditor::SpawnMainTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory","Main","Item Definition"))
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()[ BuildLeftPanel() ]
			]
			+ SSplitter::Slot()
			.Value(0.4f)
			[
				BuildPreviewWidget()
			]
		];
}

// removed CreateFilteredDetails (legacy)
// TSharedRef<IDetailsView> FYIItemDefinitionEditor::CreateFilteredDetails(const TArray<FName>& AllowedProps)
/*
	FPropertyEditorModule& P = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args;
	Args.bAllowSearch = true;
	Args.bHideSelectionTip = true;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	TSharedRef<IDetailsView> View = P.CreateDetailView(Args);
	View->SetObject(EditingAsset, true);
	// TODO: Implement property filtering via detail customization for per-tab visibility.
	return View;
}
*/

/* Removed old tab spawners
TSharedRef<SDockTab> FYIItemDefinitionEditor::SpawnOverviewTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory","Overview","Overview"))
		[
			OverviewDetails.ToSharedRef()
		];
}

TSharedRef<SDockTab> FYIItemDefinitionEditor::SpawnCapabilitiesTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory","Capabilities","Capabilities"))
		[
			CapabilitiesDetails.ToSharedRef()
		];
}

TSharedRef<SDockTab> FYIItemDefinitionEditor::SpawnAttributesTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory","Attributes","Attributes"))
		[
			AttributesDetails.ToSharedRef()
		];
}

TSharedRef<SDockTab> FYIItemDefinitionEditor::SpawnUnlockTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory","Unlock","Unlock"))
		[
			UnlockDetails.ToSharedRef()
		];
}
*/

TSharedRef<SWidget> FYIItemDefinitionEditor::BuildLeftPanel()
{
	// Simplified: remove Item Attribute Keywords UI; show details only
	return MainDetails.ToSharedRef();
}

// Removed: Item Attribute Keywords UI
void FYIItemDefinitionEditor::RefreshAttributeKeywordsList_REMOVED()
{
	// no-op
}

// Removed legacy Item Attribute Keywords UI
auto FYIItemDefinitionEditor_OnAttrDrop_REMOVED = 0;
FReply FYIItemDefinitionEditor::OnAttrDrop_REMOVED(const FGeometry& Geo, const FDragDropEvent& Evt)
{
	return FReply::Unhandled();
}

// Removed legacy Item Attribute Keywords UI
FReply FYIItemDefinitionEditor::OnClearAttributesClicked_REMOVED()
{
	return FReply::Unhandled();
}

TSharedRef<SWidget> FYIItemDefinitionEditor::BuildPreviewWidget()
{
	return SNew(SAssetDropTarget)
		.OnAreAssetsAcceptableForDrop(this, &FYIItemDefinitionEditor::AreModsAssetsAcceptable)
		.OnAssetsDropped(this, &FYIItemDefinitionEditor::OnModsAssetsDropped)
		[ SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				BuildItemCard()
			]
		];
}

TSharedRef<SWidget> FYIItemDefinitionEditor::BuildItemCard()
{
	// DS-style card inside a bordered, slightly tinted panel based on rarity
	TSharedRef<SVerticalBox> VBox = SNew(SVerticalBox);

	auto MakeSectionHeader = [](const FText& Label)
	{
		return SNew(STextBlock)
			.Text(Label)
			.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
			.ColorAndOpacity(FLinearColor(0.9f,0.9f,1.f));
	};

// Header: Name with affix links (prefixes, base name colored by rarity, suffixes)
	{
		TSharedRef<SHorizontalBox> TitleBox = SNew(SHorizontalBox);
		if (EditingAsset)
		{
			// Prefix affixes
			for (const TSoftObjectPtr<UYIAffixAsset>& SoftA : EditingAsset->TemplateAffixes)
			{
				UYIAffixAsset* A = SoftA.IsValid() ? SoftA.Get() : SoftA.LoadSynchronous();
				if (A && A->Kind == EYIAffixKind::Prefix)
				{
					TitleBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(2,0,4,0)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
						.OnClicked_Lambda([A]() {
							if (A)
							{
								TArray<UObject*> ToOpen{A};
								GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ToOpen);
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock).Text(A->DisplayName).ColorAndOpacity(FLinearColor(0.35f,0.65f,1.f)).Font(FAppStyle::Get().GetFontStyle("NormalFont"))
						]
					];
				}
			}
			// Base name
			TitleBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(6,0,6,0)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return EditingAsset ? EditingAsset->DisplayName : FText::GetEmpty(); })
				.Font(FAppStyle::Get().GetFontStyle("HeadingExtraSmall"))
				.ColorAndOpacity_Lambda([this]() { return EditingAsset ? UYIInventoryBlueprintLibrary::GetColorForRarityTag(EditingAsset->RarityTag) : FLinearColor::White; })
			];
			// Suffix affixes
			for (const TSoftObjectPtr<UYIAffixAsset>& SoftA : EditingAsset->TemplateAffixes)
			{
				UYIAffixAsset* A = SoftA.IsValid() ? SoftA.Get() : SoftA.LoadSynchronous();
				if (A && A->Kind == EYIAffixKind::Suffix)
				{
					TitleBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(6,0,2,0)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
						.OnClicked_Lambda([A]() {
							if (A)
							{
								TArray<UObject*> ToOpen{A};
								GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ToOpen);
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock).Text(A->DisplayName).ColorAndOpacity(FLinearColor(0.35f,0.65f,1.f)).Font(FAppStyle::Get().GetFontStyle("NormalFont"))
						]
					];
				}
			}
		}
		VBox->AddSlot().AutoHeight().Padding(8,6)[ TitleBox ];
	}

	// Description
	VBox->AddSlot().AutoHeight().Padding(8,2)
	[
		SNew(STextBlock)
		.Text_Lambda([this]() { return EditingAsset ? EditingAsset->Description : FText::GetEmpty(); })
		.AutoWrapText(true)
	];

	// Template affix descriptors (clickable to open affix asset)
	VBox->AddSlot().AutoHeight().Padding(8,2)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","AffixHeader","Affixes:"))
		]
	];
	if (EditingAsset && EditingAsset->TemplateAffixes.Num() > 0)
	{
		for (const TSoftObjectPtr<UYIAffixAsset>& SoftA : EditingAsset->TemplateAffixes)
		{
			UYIAffixAsset* A = SoftA.IsValid() ? SoftA.Get() : SoftA.LoadSynchronous();
			if (!A) continue;
			VBox->AddSlot().AutoHeight().Padding(8,0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
					.OnClicked_Lambda([A]() {
						if (A) { TArray<UObject*> ToOpen{A}; GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ToOpen); }
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(A->DisplayName).ColorAndOpacity(FLinearColor(0.35f,0.65f,1.f)).Font(FAppStyle::Get().GetFontStyle("SmallFont"))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6,0,0,0)
				[
					SNew(STextBlock).Text(A->Description).AutoWrapText(true).Font(FAppStyle::Get().GetFontStyle("SmallFont"))
				]
			];
		}
	}


	// Item Requirements (per-item requirement objects)
	VBox->AddSlot().AutoHeight().Padding(8,8)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeSectionHeader(NSLOCTEXT("YOLOInventory","ItemReq","Item Requirements")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,2)
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				if (!EditingAsset) return FText::GetEmpty();
				FString Line;
				FYIRequirementContext Ctx; /* empty context for label building; editor preview may not have ASC */
				for (int32 i=0; i<EditingAsset->Requirements.Num(); ++i)
				{
					if (const UYIRequirement* R = EditingAsset->Requirements[i])
					{
						if (!Line.IsEmpty()) Line += TEXT(", ");
						Line += R->GetDisplayText(Ctx).ToString();
					}
				}
				return FText::FromString(Line);
			})
		]
	];


	// Attribute Mods (drag-drop)
	VBox->AddSlot().AutoHeight().Padding(8,8)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Mods","Attributes"))
			.Font(FAppStyle::Get().GetFontStyle("BoldFont")) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(AttrModsList, SVerticalBox)
		]
	];

	// Special properties removed (capabilities)

	// Variants Section
	VBox->AddSlot().AutoHeight().Padding(8,8)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Variants","Variants (Drag & Drop UYIItemVariant)"))
			.Font(FAppStyle::Get().GetFontStyle("BoldFont")) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SAssetDropTarget)
			.OnAreAssetsAcceptableForDrop_Lambda([](TArrayView<const FAssetData> Assets){
				for (const FAssetData& AD : Assets) { if (AD.GetClass() && AD.GetClass()->IsChildOf(UYIItemVariantAsset::StaticClass())) return true; }
				return false; })
			.OnAssetsDropped_Lambda([this](const FDragDropEvent& Evt, TArrayView<const FAssetData> Assets){
				if (!EditingAsset) return; bool bLinked=false;
				for (const FAssetData& AD : Assets)
				{
					if (AD.GetClass() && AD.GetClass()->IsChildOf(UYIItemVariantAsset::StaticClass()))
					{
						if (UYIItemVariantAsset* Var = Cast<UYIItemVariantAsset>(AD.GetAsset()))
						{
							Var->Modify();
							Var->BaseDefinition = EditingAsset;
							Var->MarkPackageDirty();
							
							bLinked = true;
						}
					}
				}
				if (bLinked) { /* trigger UI refresh by rebuilding text */ }
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","VariantHint","Drop a Variant asset here to link it to this item")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (!EditingAsset) return NSLOCTEXT("YOLOInventory","NoVariants","No variants");
						// Discover variants by scanning AssetRegistry and filtering BaseDefinition == this
						FARFilter Filter; 
						Filter.ClassPaths.Add(UYIItemVariantAsset::StaticClass()->GetClassPathName());
						Filter.bRecursiveClasses = true;
						TArray<FAssetData> Out;
						FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
						ARM.Get().GetAssets(Filter, Out);
						int32 Count = 0;
						for (const FAssetData& AD : Out)
						{
							if (UYIItemVariantAsset* Var = Cast<UYIItemVariantAsset>(AD.GetAsset()))
							{
								if (Var->BaseDefinition.IsValid() && Var->BaseDefinition.Get() == EditingAsset)
								{
									++Count;
								}
							}
						}
						return FText::FromString(Count > 0 ? FString::FromInt(Count) + TEXT(" variant(s)") : TEXT("No variants"));
					})
				]
			]
		]
	];

	// Evolution Section (pick an EvolutionPath)
	VBox->AddSlot().AutoHeight().Padding(8,8)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Evolution","Evolution Path")).Font(FAppStyle::Get().GetFontStyle("BoldFont")) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SAssetDropTarget)
			.OnAreAssetsAcceptableForDrop_Lambda([](TArrayView<const FAssetData> Assets){
				for (const FAssetData& AD : Assets) { if (AD.GetClass() && AD.GetClass()->IsChildOf(UYIEvolutionPath::StaticClass())) return true; }
				return false; })
			.OnAssetsDropped_Lambda([](const FDragDropEvent& Evt, TArrayView<const FAssetData> Assets){ /* no-op: discovery-only linkage */ })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","EvoHint","Drop an EvolutionPath asset here")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","None","None"))
				]
			]
		]
	];

	// Affixes section: template preview + pool sampling
	VBox->AddSlot().AutoHeight().Padding(8,8)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Affixes","Affixes")).Font(FAppStyle::Get().GetFontStyle("BoldFont")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,4)
		[
			SNew(SVerticalBox)
			// Template affixes summary
			+ SVerticalBox::Slot().AutoHeight().Padding(0,2)
			[
				SNew(STextBlock).Text_Lambda([this]() {
					if (!EditingAsset) return FText::GetEmpty();
					FText Lines;
					for (const TSoftObjectPtr<UYIAffixAsset>& Tmpl : EditingAsset->TemplateAffixes)
					{
						if (UYIAffixAsset* A = Tmpl.IsValid() ? Tmpl.Get() : Tmpl.LoadSynchronous())
						{
							FText Line = !A->TooltipFormat.IsEmpty() ? A->TooltipFormat : A->DisplayName;
							Lines = FText::FromString(Lines.ToString() + (Lines.IsEmpty() ? TEXT("") : TEXT("\n")) + Line.ToString());
						}
					}
					return Lines;
				})
			]
			// Sampling controls
			+ SVerticalBox::Slot().AutoHeight().Padding(0,8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2,0)[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Level","Level:")) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2,0)
				[
					SNew(SSpinBox<int32>)
					.MinValue(0)
					.MaxValue(9999)
					.Value_Lambda([this]() { return SampleLevel; })
					.OnValueChanged_Lambda([this](int32 V) { SampleLevel = V; })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8,0)[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Seed","Seed:")) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2,0)
				[
					SNew(SSpinBox<int32>)
					.MinValue(0)
					.MaxValue(INT32_MAX)
					.Value_Lambda([this]() { return SampleSeed; })
					.OnValueChanged_Lambda([this](int32 V) { SampleSeed = V; })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8,0)[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Prefixes","Prefixes:")) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2,0)
				[
					SNew(SSpinBox<int32>)
					.MinValue(0)
					.MaxValue(10)
					.Value_Lambda([this]() { return SampleNumPrefixes; })
					.OnValueChanged_Lambda([this](int32 V) { SampleNumPrefixes = V; })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8,0)[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Suffixes","Suffixes:")) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2,0)
				[
					SNew(SSpinBox<int32>)
					.MinValue(0)
					.MaxValue(10)
					.Value_Lambda([this]() { return SampleNumSuffixes; })
					.OnValueChanged_Lambda([this](int32 V) { SampleNumSuffixes = V; })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).Padding(8,0)
				[
					SNew(SButton).Text(NSLOCTEXT("YOLOInventory","SampleButton","Sample"))
					.OnClicked_Lambda([this]() -> FReply { OnSampleAffixesClicked(); return FReply::Handled(); })
				]
			]
			// Sample result list
			+ SVerticalBox::Slot().AutoHeight().Padding(0,4)
			[
				SAssignNew(SampleAffixList, SVerticalBox)
			]
		]
	];

	// Footer: Unique code
	VBox->AddSlot().AutoHeight().Padding(8,8)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Code","Code:")) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(8,0)
		[
			SNew(STextBlock).Text_Lambda([this]()
			{
				return EditingAsset ? FText::AsNumber(EditingAsset->UniqueCode) : FText::GetEmpty();
			})
		]
	];

	RefreshAttributeModsList();
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryTop"))
		.BorderBackgroundColor_Lambda([this](){
			FLinearColor C = FLinearColor(0.08f,0.08f,0.1f,0.6f);
			// Note: Rarity has been removed from item definition; keep a neutral but themed tint
			return C;
		})
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()[ VBox ]
		];
}

void FYIItemDefinitionEditor::OnModsAssetsDropped(const FDragDropEvent& Evt, TArrayView<FAssetData> InAssets)
{
	if (!EditingAsset) return;
	bool bAdded=false;
	for (const FAssetData& AD : InAssets)
	{
		if (AD.GetClass() && AD.GetClass()->IsChildOf(UYIAttributeModAsset::StaticClass()))
		{
			EditingAsset->Modify();
			EditingAsset->AttributeMods.AddUnique(TSoftObjectPtr<UYIAttributeModAsset>(AD.ToSoftObjectPath()));
			bAdded=true;
		}
	}
	if (bAdded) RefreshAttributeModsList();
}

bool FYIItemDefinitionEditor::AreModsAssetsAcceptable(TArrayView<FAssetData> InAssets)
{
	for (const FAssetData& AD : InAssets)
	{
		if (AD.GetClass() && AD.GetClass()->IsChildOf(UYIAttributeModAsset::StaticClass()))
		{
			return true;
		}
	}
	return false;
}

bool FYIItemDefinitionEditor::OnModsDragOver(const FGeometry& Geo, const FDragDropEvent& Evt)
{
	auto Op = Evt.GetOperationAs<FAssetDragDropOp>();
	if (!Op.IsValid()) return false;
	for (const FAssetData& AD : Op->GetAssets())
	{
		if (AD.GetClass() && AD.GetClass()->IsChildOf(UYIAttributeModAsset::StaticClass())) return true;
	}
	return false;
}

void FYIItemDefinitionEditor::OnSampleAffixesClicked()
{
	if (!EditingAsset) return;
	if (!SampleAffixList.IsValid()) return;
	SampleAffixList->ClearChildren();

	FYIBagItem Tmp;
	Tmp.Item.Definition = EditingAsset;
	Tmp.Size = EditingAsset->DefaultSize;

	UYIInventoryBlueprintLibrary::GenerateAffixesForInstance(Tmp, SampleLevel, SampleSeed, SampleNumPrefixes, SampleNumSuffixes);

	for (const FYIAffixInstance& A : Tmp.Item.Affixes)
	{
		UYIAffixAsset* Src = A.Source.IsValid() ? A.Source.Get() : A.Source.LoadSynchronous();
		FText Line;
		if (Src)
		{
			if (!Src->TooltipFormat.IsEmpty())
			{
				FFormatNamedArguments Args; Args.Add(TEXT("0"), FText::AsNumber(A.RolledValue));
				Line = FText::Format(Src->TooltipFormat, Args);
			}
			else if (!A.DisplayNameCache.IsEmpty())
			{
				Line = A.DisplayNameCache;
			}
			else
			{
				Line = Src->DisplayName;
			}
		}
		else
		{
			Line = FText::FromString(TEXT("(Invalid affix)"));
		}
		// Affix clickable line
		SampleAffixList->AddSlot().AutoHeight().Padding(2,2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.OnClicked_Lambda([Src]() {
					if (Src) { TArray<UObject*> ToOpen{Src}; GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ToOpen); }
					return FReply::Handled();
				})
				[
					SNew(STextBlock).Text(Line).ColorAndOpacity(FLinearColor(0.35f,0.65f,1.f)).Font(FAppStyle::Get().GetFontStyle("NormalFont"))
				]
			]
		];
		// List attribute mods for this affix (if any) - attribute name clickable
		if (Src && Src->AttributeMods.Num() > 0)
		{
			for (const TSoftObjectPtr<UYIAttributeModAsset>& ModSoft : Src->AttributeMods)
			{
				if (UYIAttributeModAsset* Mod = ModSoft.IsValid() ? ModSoft.Get() : ModSoft.LoadSynchronous())
				{
					FLinearColor AttrColor = FLinearColor::White;
					FText AttrName = FText::FromString(TEXT("<Attr>"));
					if (Mod->Attribute.IsValid())
					{
						if (UYIAttributeDef* Def = Mod->Attribute.LoadSynchronous())
						{
							AttrName = !Def->DisplayName.IsEmpty() ? Def->DisplayName : FText::FromString(Def->GetName());
							AttrColor = Def->DisplayColor;
						}
					}
					SampleAffixList->AddSlot().AutoHeight().Padding(8,0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Adds %g to "), Mod->Magnitude))).Font(FAppStyle::Get().GetFontStyle("SmallFont")) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[
							SNew(SButton)
							.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
							.OnClicked_Lambda([Mod]() {
								if (Mod) { TArray<UObject*> ToOpen{Mod}; GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ToOpen); }
								return FReply::Handled();
							})
							[
								SNew(STextBlock).Text(AttrName).ColorAndOpacity(AttrColor).Font(FAppStyle::Get().GetFontStyle("SmallFont"))
							]
						]
					];
			}
			}
		}
	}
}

FReply FYIItemDefinitionEditor::OnModsDrop(const FGeometry& Geo, const FDragDropEvent& Evt)
{
	auto Op = Evt.GetOperationAs<FAssetDragDropOp>();
	if (!EditingAsset || !Op.IsValid()) return FReply::Unhandled();
	bool bAdded=false;
	for (const FAssetData& AD : Op->GetAssets())
	{
		if (AD.GetClass()->IsChildOf(UYIAttributeModAsset::StaticClass()))
		{
			EditingAsset->Modify();
			EditingAsset->AttributeMods.AddUnique(TSoftObjectPtr<UYIAttributeModAsset>(AD.ToSoftObjectPath()));
			bAdded=true;
		}
	}
	if (bAdded) { RefreshAttributeModsList(); }
	return bAdded? FReply::Handled() : FReply::Unhandled();
}

void FYIItemDefinitionEditor::RefreshAttributeModsList()
{
	if (!AttrModsList.IsValid()) return;
	AttrModsList->ClearChildren();
	if (!EditingAsset) return;
	for (int32 i=0;i<EditingAsset->AttributeMods.Num();++i)
	{
		TSoftObjectPtr<UYIAttributeModAsset> Ref = EditingAsset->AttributeMods[i];
		UYIAttributeModAsset* Mod = Ref.LoadSynchronous();
		// Resolve attribute def for display name and color
		FLinearColor AttrColor = FLinearColor::White;
		FText AttrName = FText::FromString(TEXT("<Attr>"));
		if (Mod && Mod->Attribute.IsValid())
		{
			if (UYIAttributeDef* Def = Mod->Attribute.LoadSynchronous())
			{
				AttrName = !Def->DisplayName.IsEmpty()? Def->DisplayName : FText::FromString(Def->GetName());
				AttrColor = Def->DisplayColor;
			}
		}
		// Make a concise line like: Strength +2
		const float Mag = Mod? Mod->Magnitude : 0.f;
		FString Sign = Mag >= 0.f? TEXT("+") : TEXT("");
		FText MagText = FText::FromString(Sign + FString::SanitizeFloat(Mag, 0));

		AttrModsList->AddSlot().AutoHeight().Padding(0,2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.OnClicked_Lambda([Ref]() {
					if (UYIAttributeModAsset* M = Ref.LoadSynchronous())
					{
						if (M->Attribute.IsValid())
						{
							if (UObject* Attr = M->Attribute.LoadSynchronous()) { TArray<UObject*> ToOpen{Attr}; GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ToOpen); }
							else { TArray<UObject*> ToOpen{M}; GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ToOpen); }
						}
						else { TArray<UObject*> ToOpen{M}; GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ToOpen); }
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock).Text(AttrName).ColorAndOpacity(FLinearColor::White)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6,0,0,0).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(MagText).ColorAndOpacity(AttrColor)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory","Open","Open"))
				.OnClicked_Lambda([Ref]()
				{
					if (UObject* Obj = Ref.LoadSynchronous())
					{
						TArray<UObject*> ToOpen{Obj};
						GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ToOpen);
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory","Remove","Remove"))
				.OnClicked_Lambda([this,i]()
				{
					if (!EditingAsset) return FReply::Unhandled();
					EditingAsset->Modify();
					EditingAsset->AttributeMods.RemoveAt(i);
					RefreshAttributeModsList();
					return FReply::Handled();
				})
			]
		];
	}
}
