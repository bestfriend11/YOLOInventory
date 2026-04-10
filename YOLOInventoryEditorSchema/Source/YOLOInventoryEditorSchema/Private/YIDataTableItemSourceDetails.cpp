#include "YIDataTableItemSourceDetails.h"

#include "Data/YIDataTableItemSource.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/DataTable.h"
#include "Engine/Texture.h"
#include "GameplayTagsManager.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "IDetailChildrenBuilder.h"
#include "Misc/PackageName.h"
#include "SourceCodeNavigation.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"
#include "Algo/Unique.h"
#include "StructUtils/InstancedStruct.h"
#include "YIInlineMappingResolvers.h"
#include "YIItemFragments.h"

namespace
{
static FProperty* FindPropertyByAuthoredNameLocal(const UStruct* OwnerStruct, const FName FieldName)
{
	if (!OwnerStruct || FieldName.IsNone())
	{
		return nullptr;
	}

	const FString Wanted = FieldName.ToString();
	for (TFieldIterator<FProperty> It(OwnerStruct); It; ++It)
	{
		FProperty* Prop = *It;
		if (Prop && Prop->GetAuthoredName().Equals(Wanted, ESearchCase::IgnoreCase))
		{
			return Prop;
		}
	}
	return nullptr;
}

static FProperty* FindPropertyByAuthoredPathLocal(const UStruct* OwnerStruct, const FString& FieldPath)
{
	if (!OwnerStruct || FieldPath.IsEmpty())
	{
		return nullptr;
	}

	TArray<FString> Segments;
	FieldPath.ParseIntoArray(Segments, TEXT("."), true);
	if (Segments.Num() == 0)
	{
		Segments.Add(FieldPath);
	}

	const UStruct* CurrentStruct = OwnerStruct;
	FProperty* CurrentProperty = nullptr;
	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		CurrentProperty = FindPropertyByAuthoredNameLocal(CurrentStruct, FName(*Segments[Index]));
		if (!CurrentProperty)
		{
			return nullptr;
		}

		const bool bLast = (Index == Segments.Num() - 1);
		if (!bLast)
		{
			const FStructProperty* StructProp = CastField<FStructProperty>(CurrentProperty);
			if (!StructProp || !StructProp->Struct)
			{
				return nullptr;
			}
			CurrentStruct = StructProp->Struct;
		}
	}

	return CurrentProperty;
}

static bool ShouldExpandStructLocal(const UScriptStruct* StructType, int32 Depth)
{
	if (!StructType || Depth >= 2)
	{
		return false;
	}
	if (StructType == FGameplayTag::StaticStruct())
	{
		return false;
	}
	return true;
}

static void CollectStructFieldOptionsLocal(const UStruct* OwnerStruct, TArray<TSharedPtr<FString>>& OutOptions)
{
	OutOptions.Reset();
	if (!OwnerStruct)
	{
		return;
	}

	TArray<FString> Names;
	TFunction<void(const UStruct*, const FString&, int32)> CollectRecursive;
	CollectRecursive = [&](const UStruct* Struct, const FString& Prefix, int32 Depth)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property)
			{
				continue;
			}

			const FString LocalName = Property->GetAuthoredName();
			const FString FullName = Prefix.IsEmpty()
				? LocalName
				: FString::Printf(TEXT("%s.%s"), *Prefix, *LocalName);
			Names.Add(FullName);

			if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				if (ShouldExpandStructLocal(StructProp->Struct, Depth))
				{
					CollectRecursive(StructProp->Struct, FullName, Depth + 1);
				}
			}
		}
	};

	CollectRecursive(OwnerStruct, FString(), 0);
	Names.Sort();
	Names.SetNum(Algo::Unique(Names));
	for (const FString& Name : Names)
	{
		OutOptions.Add(MakeShared<FString>(Name));
	}
}

static bool IsMappableFragmentFieldLocal(const FProperty* Property)
{
	return Property
		&& !Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated)
		&& !Property->HasMetaData(TEXT("YIInlineMapIgnore"));
}

static UScriptStruct* ResolveStructFromPathLocal(const FString& StructPath)
{
	if (StructPath.IsEmpty())
	{
		return nullptr;
	}
	if (UScriptStruct* Found = FindObject<UScriptStruct>(nullptr, *StructPath))
	{
		return Found;
	}
	return LoadObject<UScriptStruct>(nullptr, *StructPath);
}

static FString MakeReadableFragmentNameLocal(const UScriptStruct* FragmentStruct)
{
	if (!FragmentStruct)
	{
		return TEXT("Fragment");
	}

	const FString MetaDisplayName = FragmentStruct->GetMetaData(TEXT("DisplayName"));
	if (!MetaDisplayName.IsEmpty())
	{
		return MetaDisplayName;
	}

	FString Name = FragmentStruct->GetName();
	Name.RemoveFromStart(TEXT("FYI"));
	Name.RemoveFromStart(TEXT("YI"));
	Name.RemoveFromEnd(TEXT("DefinitionFragment"));
	Name.RemoveFromEnd(TEXT("Fragment"));
	Name.RemoveFromStart(TEXT("Item"));
	Name = Name.TrimStartAndEnd();
	return Name.IsEmpty() ? FragmentStruct->GetName() : Name;
}

static FString MakeReadableFragmentNameFromPathLocal(const FString& StructPath)
{
	if (UScriptStruct* Struct = ResolveStructFromPathLocal(StructPath))
	{
		return MakeReadableFragmentNameLocal(Struct);
	}
	return FPackageName::ObjectPathToObjectName(StructPath);
}

static void CollectFragmentStructOptionsLocal(TArray<TSharedPtr<FString>>& OutOptions)
{
	OutOptions.Reset();
	TArray<FString> Paths;
	TSet<FString> SeenPaths;
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		UScriptStruct* Struct = *It;
		if (!Struct || !Struct->IsChildOf(FYIItemDefinitionFragmentBase::StaticStruct()) || Struct == FYIItemDefinitionFragmentBase::StaticStruct())
		{
			continue;
		}
		const FString Path = Struct->GetPathName();
		if (Path.Contains(TEXT("TRASH")) || Path.Contains(TEXT("REINST")))
		{
			continue;
		}
		if (SeenPaths.Contains(Path))
		{
			continue;
		}
		SeenPaths.Add(Path);
		Paths.Add(Path);
	}

	auto AddIfMissing = [&Paths, &SeenPaths](UScriptStruct* Struct)
	{
		if (!Struct)
		{
			return;
		}
		const FString Path = Struct->GetPathName();
		if (Path.IsEmpty() || SeenPaths.Contains(Path))
		{
			return;
		}
		SeenPaths.Add(Path);
		Paths.Add(Path);
	};
	AddIfMissing(FYIItemUIDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemClassificationDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemAudioDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemLayoutDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemStackingDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemRulesDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemContainerDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemAttributeModsDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemPickupDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemWeightDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemEquipmentDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemAffixDefinitionFragment::StaticStruct());

	Paths.Sort();
	for (const FString& Path : Paths)
	{
		OutOptions.Add(MakeShared<FString>(Path));
	}
}

static bool IsFragmentStructUniqueLocal(const UScriptStruct* FragmentStruct)
{
	return YIIsDefinitionFragmentStructUnique(FragmentStruct);
}

static void GetPropertyTypeInfoLocal(const FProperty* Property, FString& OutTypeName, FLinearColor& OutColor)
{
	OutTypeName = TEXT("Any");
	OutColor = FLinearColor(0.20f, 0.20f, 0.20f, 1.0f);
	if (!Property)
	{
		return;
	}

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	if (K2Schema)
	{
		FEdGraphPinType PinType;
		K2Schema->ConvertPropertyToPinType(Property, PinType);
		OutColor = K2Schema->GetPinTypeColor(PinType);
	}

	OutTypeName = Property->GetCPPType();
}

static FString ConversionToLabelLocal(EYIFieldMappingConversion Conversion)
{
	switch (Conversion)
	{
	case EYIFieldMappingConversion::ToName: return TEXT("To Name");
	case EYIFieldMappingConversion::ToText: return TEXT("To Text");
	case EYIFieldMappingConversion::ToInt: return TEXT("To Int");
	case EYIFieldMappingConversion::ToFloat: return TEXT("To Float");
	case EYIFieldMappingConversion::BoolFromInt: return TEXT("Bool from Int>0");
	case EYIFieldMappingConversion::BoolFromText: return TEXT("Bool from Text");
	case EYIFieldMappingConversion::ToEnum: return TEXT("To Enum");
	case EYIFieldMappingConversion::ToGameplayTag: return TEXT("To Gameplay Tag");
	case EYIFieldMappingConversion::ToSoftObject: return TEXT("To Soft Object");
	case EYIFieldMappingConversion::ToSoftTexture: return TEXT("To Texture (Soft)");
	case EYIFieldMappingConversion::Vector2DFromXY: return TEXT("Vector2D from XY Fields");
	case EYIFieldMappingConversion::None:
	default:
		return TEXT("None");
	}
}

static EYIFieldMappingConversion LabelToConversionLocal(const FString& Label)
{
	if (Label == TEXT("To Name")) return EYIFieldMappingConversion::ToName;
	if (Label == TEXT("To Text")) return EYIFieldMappingConversion::ToText;
	if (Label == TEXT("To Int")) return EYIFieldMappingConversion::ToInt;
	if (Label == TEXT("To Float")) return EYIFieldMappingConversion::ToFloat;
	if (Label == TEXT("Bool from Int>0")) return EYIFieldMappingConversion::BoolFromInt;
	if (Label == TEXT("Bool from Text")) return EYIFieldMappingConversion::BoolFromText;
	if (Label == TEXT("To Enum")) return EYIFieldMappingConversion::ToEnum;
	if (Label == TEXT("To Gameplay Tag")) return EYIFieldMappingConversion::ToGameplayTag;
	if (Label == TEXT("To Soft Object")) return EYIFieldMappingConversion::ToSoftObject;
	if (Label == TEXT("To Texture (Soft)")) return EYIFieldMappingConversion::ToSoftTexture;
	if (Label == TEXT("Vector2D from XY Fields")) return EYIFieldMappingConversion::Vector2DFromXY;
	return EYIFieldMappingConversion::None;
}

static EYIFieldMappingConversion GuessConversionForPropsLocal(const FProperty* SourceProp, const FProperty* TargetProp)
{
	if (!SourceProp || !TargetProp)
	{
		return EYIFieldMappingConversion::None;
	}
	if (SourceProp->GetClass() == TargetProp->GetClass())
	{
		return EYIFieldMappingConversion::None;
	}
	if (CastField<FBoolProperty>(TargetProp))
	{
		if (CastField<FNumericProperty>(SourceProp))
		{
			return EYIFieldMappingConversion::BoolFromInt;
		}
		if (CastField<FStrProperty>(SourceProp) || CastField<FNameProperty>(SourceProp) || CastField<FTextProperty>(SourceProp))
		{
			return EYIFieldMappingConversion::BoolFromText;
		}
	}
	if (CastField<FNameProperty>(TargetProp))
	{
		return EYIFieldMappingConversion::ToName;
	}
	if (CastField<FTextProperty>(TargetProp))
	{
		return EYIFieldMappingConversion::ToText;
	}
	if (CastField<FEnumProperty>(TargetProp))
	{
		return EYIFieldMappingConversion::ToEnum;
	}
	if (const FNumericProperty* TargetNum = CastField<FNumericProperty>(TargetProp))
	{
		if (TargetNum->IsEnum())
		{
			return EYIFieldMappingConversion::ToEnum;
		}
		if (TargetNum->IsInteger())
		{
			return EYIFieldMappingConversion::ToInt;
		}
		return EYIFieldMappingConversion::ToFloat;
	}
	if (const FStructProperty* TargetStruct = CastField<FStructProperty>(TargetProp))
	{
		if (TargetStruct->Struct == FGameplayTag::StaticStruct())
		{
			return EYIFieldMappingConversion::ToGameplayTag;
		}
	}
	if (const FSoftObjectProperty* TargetSoftObj = CastField<FSoftObjectProperty>(TargetProp))
	{
		if (TargetSoftObj->PropertyClass && TargetSoftObj->PropertyClass->IsChildOf(UTexture::StaticClass()))
		{
			return EYIFieldMappingConversion::ToSoftTexture;
		}
		return EYIFieldMappingConversion::ToSoftObject;
	}
	return EYIFieldMappingConversion::None;
}

static bool TryGetFragmentFieldGuidanceLocal(const FProperty* Property, FString& OutGuidance)
{
	OutGuidance.Reset();
	if (!Property)
	{
		return false;
	}

	const UStruct* Owner = Property->GetOwnerStruct();
	const FString OwnerName = Owner ? Owner->GetName() : FString();
	const FName FieldName = Property->GetFName();

	auto Set = [&OutGuidance](const TCHAR* Text)
	{
		OutGuidance = Text;
		return true;
	};

	if (FieldName == TEXT("FragmentTag"))
	{
		return Set(TEXT("Semantic lookup key for runtime systems. Bind when gameplay resolves fragments by tag. Optional if systems resolve by exact fragment type."));
	}
	if (FieldName == TEXT("FragmentName"))
	{
		return Set(TEXT("Designer/debug label. Usually optional. Useful when multiple custom fragments are present."));
	}
	if (FieldName == TEXT("Properties"))
	{
		return Set(TEXT("PropertyBag payload for no-C++ fragment authoring. Inline mappings can target Properties.<FieldName>. Bind only fields your runtime systems actually consume."));
	}

	if (OwnerName == TEXT("YIItemUIDefinitionFragment"))
	{
		if (FieldName == TEXT("DisplayName")) return Set(TEXT("Primary player-facing item name. Usually bind this. Dashboards and tooltips rely on it."));
		if (FieldName == TEXT("Description")) return Set(TEXT("Long description / flavor text. Optional unless your UI displays inspect/tooltips."));
		if (FieldName == TEXT("Icon")) return Set(TEXT("UI icon soft reference. Bind for inventory/shop/tooltips with images."));
	}
	if (OwnerName == TEXT("YIItemClassificationDefinitionFragment"))
	{
		if (FieldName == TEXT("ItemType")) return Set(TEXT("Primary gameplay classification tag (e.g., Item.Weapon.Sword). Bind when systems branch on type."));
		if (FieldName == TEXT("Tags")) return Set(TEXT("Additional classification tags. Optional; bind only tags used by your systems."));
		if (FieldName == TEXT("RarityTag")) return Set(TEXT("Rarity tag used by UI/economy/drop rules. Optional if your game has no rarity system."));
	}
	if (OwnerName == TEXT("YIItemLayoutDefinitionFragment"))
	{
		if (FieldName == TEXT("DefaultSize")) return Set(TEXT("Grid footprint size. Required for grid inventories; irrelevant for list inventories."));
		if (FieldName == TEXT("bAllowRotation")) return Set(TEXT("Grid rotation support flag. Ignore if your inventory UI does not support rotation."));
	}
	if (OwnerName == TEXT("YIItemEquipmentDefinitionFragment"))
	{
		if (FieldName == TEXT("PrimaryEquipSlot")) return Set(TEXT("Preferred slot tag for equip logic. Bind for equipable items."));
		if (FieldName == TEXT("OccupiedSlots")) return Set(TEXT("All occupied equipment slots. Optional for single-slot items; important for multi-slot gear."));
	}
	if (OwnerName == TEXT("YIItemStackingDefinitionFragment"))
	{
		if (FieldName == TEXT("MaxStackCount")) return Set(TEXT("Maximum stack size. If this fragment is absent, the item is non-stackable."));
		if (FieldName == TEXT("bUseRiskChecks")) return Set(TEXT("Safety checks for stackability of mutable items. Keep enabled unless you intentionally bypass safeguards."));
	}
	if (OwnerName == TEXT("YIItemContainerDefinitionFragment"))
	{
		if (FieldName == TEXT("bIsContainerItem")) return Set(TEXT("Marks item as nested container (bag-in-bag). Bind when item should hold another bag."));
		if (FieldName == TEXT("ContainerTemplateBag")) return Set(TEXT("Optional template nested bag. Use for predefined container layouts."));
		if (FieldName == TEXT("ContainerDefaultGridSize")) return Set(TEXT("Fallback nested bag size when no template bag is assigned."));
	}

	return false;
}

static FText BuildInlineMappingTargetFieldTooltipLocal(const FYIFieldMapping* Mapping, const FProperty* TargetProperty)
{
	FString Text;
	if (TargetProperty)
	{
		const FString NativeTooltip = TargetProperty->GetToolTipText().ToString();
		if (!NativeTooltip.IsEmpty())
		{
			Text += NativeTooltip;
		}

		if (!Text.IsEmpty()) Text += TEXT("\n\n");
		Text += FString::Printf(TEXT("Type: %s"), *TargetProperty->GetCPPType());

		FString Guidance;
		if (TryGetFragmentFieldGuidanceLocal(TargetProperty, Guidance) && !Guidance.IsEmpty())
		{
			Text += TEXT("\n\nUsage / Binding Guidance:\n");
			Text += Guidance;
		}
	}

	if (Mapping)
	{
		if (!Text.IsEmpty()) Text += TEXT("\n\n");
		Text += TEXT("Inline Mapping Notes:\n");
		Text += TEXT("- Bind this from CSV only if your game/system consumes it.\n");
		Text += TEXT("- Use Static when the value should be constant across rows.\n");
		if (Mapping->bTargetPropertyBagField)
		{
			Text += TEXT("- This row targets a Custom Fragment PropertyBag field (Properties.<FieldName>).\n");
		}
	}

	if (!Text.IsEmpty())
	{
		Text += TEXT("\n\nClick to open native source.");
	}

	return FText::FromString(Text);
}
}

TSharedRef<IDetailCustomization> FYIDataTableItemSourceDetails::MakeInstance()
{
	return MakeShared<FYIDataTableItemSourceDetails>();
}

void FYIDataTableItemSourceDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	CachedDetailBuilder = &DetailBuilder;
	EditedSource.Reset();

	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	for (const TWeakObjectPtr<UObject>& Obj : Objects)
	{
		if (UYIDataTableItemSource* Source = Cast<UYIDataTableItemSource>(Obj.Get()))
		{
			EditedSource = Source;
			break;
		}
	}
	if (!EditedSource.IsValid())
	{
		return;
	}

	DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UYIDataTableItemSource, InlineMappings));
	RebuildCachedOptions();

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(
		TEXT("Inline Mapping Authoring"),
		FText::FromString(TEXT("Inline Mapping Authoring")),
		ECategoryPriority::Important);

	Category.AddCustomRow(FText::FromString(TEXT("FragmentPresetAdd")))
	.WholeRowContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.55f).Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&FragmentStructOptions)
				.OnComboBoxOpening_Lambda([this]()
					{
						RebuildCachedOptions();
					})
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
					{
						return SNew(STextBlock).Text(FText::FromString(MakeReadableFragmentNameFromPathLocal(InItem.IsValid() ? *InItem : FString())));
					})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
					{
						if (NewItem.IsValid())
						{
							SelectedFragmentStructOption = NewItem;
						}
					})
				.Content()
				[
					SNew(STextBlock).Text_Lambda([this]()
						{
							if (!SelectedFragmentStructOption.IsValid())
							{
								return NSLOCTEXT("YOLOInventory", "SourceDetails_SelectFragment", "Select fragment");
							}
							return FText::FromString(MakeReadableFragmentNameFromPathLocal(*SelectedFragmentStructOption));
						})
				]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory", "SourceDetails_AddFragmentFields", "Add Fragment (+Fields)"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "SourceDetails_AddFragmentFields_TT", "Adds one mapping row for each writable field in the selected fragment."))
				.OnClicked_Lambda([this]()
					{
						AddFragmentMappingsForSelectedFragment();
						return FReply::Handled();
					})
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory", "SourceDetails_AutoMatch", "Auto Match"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "SourceDetails_AutoMatch_TT", "Auto-matches source fields by authored field name for existing fragment mappings."))
				.OnClicked_Lambda([this]()
					{
						AutoMatchExistingMappings();
						return FReply::Handled();
					})
		]
	];

	Category.AddCustomRow(FText::FromString(TEXT("MappingsTree")))
	.WholeRowContent()
	[
		BuildMappingsTreeWidget()
	];
}

void FYIDataTableItemSourceDetails::RebuildCachedOptions()
{
	FragmentStructOptions.Reset();
	SourceFieldOptions.Reset();
	ConversionOptions.Reset();
	TransformFunctionOptions.Reset();

	CollectFragmentStructOptionsLocal(FragmentStructOptions);
	if (!SelectedFragmentStructOption.IsValid()
		|| !FragmentStructOptions.ContainsByPredicate([this](const TSharedPtr<FString>& Option)
			{
				return Option.IsValid()
					&& SelectedFragmentStructOption.IsValid()
					&& **Option == **SelectedFragmentStructOption;
			}))
	{
		SelectedFragmentStructOption = FragmentStructOptions.Num() > 0
			? FragmentStructOptions[0]
			: TSharedPtr<FString>();
	}

	if (const UYIDataTableItemSource* Source = EditedSource.Get())
	{
		if (UDataTable* DataTable = Source->DataTable.LoadSynchronous())
		{
			CollectStructFieldOptionsLocal(DataTable->RowStruct, SourceFieldOptions);
		}
	}
	SourceFieldOptions.Insert(MakeShared<FString>(TEXT("<None>")), 0);

	ConversionOptions.Add(MakeShared<FString>(TEXT("None")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("To Name")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("To Text")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("To Int")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("To Float")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("Bool from Int>0")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("Bool from Text")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("To Enum")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("To Gameplay Tag")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("To Soft Object")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("To Texture (Soft)")));
	ConversionOptions.Add(MakeShared<FString>(TEXT("Vector2D from XY Fields")));

	TSharedPtr<FYITransformFunctionInfo> NoneOpt = MakeShared<FYITransformFunctionInfo>();
	NoneOpt->DisplayName = TEXT("None");
	TransformFunctionOptions.Add(NoneOpt);
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class || !Class->IsChildOf(UBlueprintFunctionLibrary::StaticClass()))
		{
			continue;
		}

		for (TFieldIterator<UFunction> FnIt(Class, EFieldIteratorFlags::IncludeSuper); FnIt; ++FnIt)
		{
			UFunction* Fn = *FnIt;
			if (!Fn || !Fn->HasMetaData(TEXT("YIInlineTransform")))
			{
				continue;
			}

			TSharedPtr<FYITransformFunctionInfo> Entry = MakeShared<FYITransformFunctionInfo>();
			Entry->Library = Class;
			Entry->FunctionName = Fn->GetFName();
			Entry->DisplayName = FString::Printf(TEXT("%s::%s"), *Class->GetName(), *Fn->GetName());
			TransformFunctionOptions.Add(Entry);
		}
	}
	TransformFunctionOptions.Sort([](const TSharedPtr<FYITransformFunctionInfo>& A, const TSharedPtr<FYITransformFunctionInfo>& B)
		{
			if (!A.IsValid() || !B.IsValid())
			{
				return A.IsValid();
			}
			if (A->FunctionName.IsNone())
			{
				return true;
			}
			if (B->FunctionName.IsNone())
			{
				return false;
			}
			return A->DisplayName < B->DisplayName;
		});
}

void FYIDataTableItemSourceDetails::RequestRefresh() const
{
	if (CachedDetailBuilder)
	{
		CachedDetailBuilder->ForceRefreshDetails();
	}
}

void FYIDataTableItemSourceDetails::AddFragmentMappingsForSelectedFragment()
{
	if (!EditedSource.IsValid() || !SelectedFragmentStructOption.IsValid())
	{
		return;
	}

	UScriptStruct* FragmentStruct = ResolveStructFromPathLocal(*SelectedFragmentStructOption);
	if (!FragmentStruct)
	{
		return;
	}

	AddFragmentMappingsForStruct(FragmentStruct);
}

void FYIDataTableItemSourceDetails::AddFragmentMappingsForStruct(const UScriptStruct* FragmentStruct)
{
	UYIDataTableItemSource* Source = EditedSource.Get();
	if (!Source || !FragmentStruct)
	{
		return;
	}

	Source->Modify();
	const bool bUnique = IsFragmentStructUniqueLocal(FragmentStruct);
	if (bUnique)
	{
		const bool bHasAny = Source->InlineMappings.ContainsByPredicate([FragmentStruct](const FYIFieldMapping& Mapping)
			{
				return Mapping.TargetLayer == EYIFieldMappingTargetLayer::StaticDefinitionFragment
					&& Mapping.TargetFragmentStruct.Get() == FragmentStruct;
			});
		if (bHasAny)
		{
			return;
		}
	}

	UDataTable* Table = Source->DataTable.LoadSynchronous();
	const UScriptStruct* RowStruct = Table ? Table->RowStruct : nullptr;

	TArray<TSharedPtr<FString>> FragmentFieldPaths;
	CollectStructFieldOptionsLocal(FragmentStruct, FragmentFieldPaths);
	for (const TSharedPtr<FString>& FieldPathPtr : FragmentFieldPaths)
	{
		if (!FieldPathPtr.IsValid())
		{
			continue;
		}

		const FString FieldPath = *FieldPathPtr;
		FProperty* TargetProp = FindPropertyByAuthoredPathLocal(FragmentStruct, FieldPath);
		if (!TargetProp || !IsMappableFragmentFieldLocal(TargetProp))
		{
			continue;
		}

		const FName TargetField(*FieldPath);
		const bool bAlreadyMapped = Source->InlineMappings.ContainsByPredicate([FragmentStruct, TargetField](const FYIFieldMapping& Mapping)
			{
				if (Mapping.TargetLayer != EYIFieldMappingTargetLayer::StaticDefinitionFragment)
				{
					return false;
				}
				if (Mapping.TargetFragmentStruct.Get() != FragmentStruct)
				{
					return false;
				}
				return YIGetResolvedTargetFieldName(Mapping).IsEqual(TargetField);
			});
		if (bAlreadyMapped)
		{
			continue;
		}

		FYIFieldMapping NewMapping;
		NewMapping.TargetLayer = EYIFieldMappingTargetLayer::StaticDefinitionFragment;
		NewMapping.TargetFragmentStruct = const_cast<UScriptStruct*>(FragmentStruct);
		NewMapping.TargetFragmentField = TargetField;
		NewMapping.TargetProperty = TargetField;

		if (RowStruct)
		{
			if (const FProperty* SourcePathProp = FindPropertyByAuthoredPathLocal(RowStruct, FieldPath))
			{
				NewMapping.SourceField = FName(*FieldPath);
				NewMapping.Conversion = GuessConversionForPropsLocal(SourcePathProp, TargetProp);
			}
			else
			{
				const FName LeafName(*TargetProp->GetAuthoredName());
				if (const FProperty* SourceLeafProp = FindPropertyByAuthoredNameLocal(RowStruct, LeafName))
				{
					NewMapping.SourceField = LeafName;
					NewMapping.Conversion = GuessConversionForPropsLocal(SourceLeafProp, TargetProp);
				}
			}
		}

		Source->InlineMappings.Add(NewMapping);
	}

	RequestRefresh();
}

void FYIDataTableItemSourceDetails::AutoMatchExistingMappings()
{
	UYIDataTableItemSource* Source = EditedSource.Get();
	if (!Source)
	{
		return;
	}

	UDataTable* Table = Source->DataTable.LoadSynchronous();
	if (!Table || !Table->RowStruct)
	{
		return;
	}

	Source->Modify();
	for (FYIFieldMapping& Mapping : Source->InlineMappings)
	{
		if (Mapping.TargetLayer != EYIFieldMappingTargetLayer::StaticDefinitionFragment || !Mapping.TargetFragmentStruct)
		{
			continue;
		}
		const FName TargetField = YIGetResolvedTargetFieldName(Mapping);
		if (TargetField.IsNone())
		{
			continue;
		}

		const FString TargetFieldPath = TargetField.ToString();
		FProperty* TargetProp = FindPropertyByAuthoredPathLocal(Mapping.TargetFragmentStruct.Get(), TargetFieldPath);
		if (!TargetProp)
		{
			continue;
		}

		if (const FProperty* SourcePathProp = FindPropertyByAuthoredPathLocal(Table->RowStruct, TargetFieldPath))
		{
			Mapping.SourceField = FName(*TargetFieldPath);
			Mapping.Conversion = GuessConversionForPropsLocal(SourcePathProp, TargetProp);
			continue;
		}

		const FName LeafName(*TargetProp->GetAuthoredName());
		if (const FProperty* SourceLeafProp = FindPropertyByAuthoredNameLocal(Table->RowStruct, LeafName))
		{
			Mapping.SourceField = LeafName;
			Mapping.Conversion = GuessConversionForPropsLocal(SourceLeafProp, TargetProp);
		}
	}

	RequestRefresh();
}

TSharedRef<SWidget> FYIDataTableItemSourceDetails::BuildMappingsTreeWidget()
{
	UYIDataTableItemSource* Source = EditedSource.Get();
	if (!Source)
	{
		return SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "SourceDetails_NoSource", "No source selected."));
	}

	TMap<FString, TArray<int32>> Grouped;
	TArray<FString> GroupKeys;

	for (int32 Index = 0; Index < Source->InlineMappings.Num(); ++Index)
	{
		const FYIFieldMapping& Mapping = Source->InlineMappings[Index];
		FString Key = TEXT("Unassigned");
		if (Mapping.TargetFragmentStruct)
		{
			Key = Mapping.TargetFragmentStruct->GetPathName();
		}
		if (!Grouped.Contains(Key))
		{
			Grouped.Add(Key, TArray<int32>());
			GroupKeys.Add(Key);
		}
		Grouped[Key].Add(Index);
	}
	GroupKeys.Sort();

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);
	if (GroupKeys.Num() == 0)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(STextBlock)
				.Text(NSLOCTEXT("YOLOInventory", "SourceDetails_NoMappingsYet", "No mappings yet. Select a fragment and click Add Fragment (+Fields)."))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f)))
		];
	}

	for (const FString& GroupKey : GroupKeys)
	{
		const TArray<int32>* Group = Grouped.Find(GroupKey);
		if (!Group)
		{
			continue;
		}

		const FString GroupLabel = MakeReadableFragmentNameFromPathLocal(GroupKey);
		TSharedRef<SVerticalBox> GroupContent = SNew(SVerticalBox);

		UScriptStruct* GroupStruct = ResolveStructFromPathLocal(GroupKey);
		TSharedPtr<TArray<TSharedPtr<FString>>> MissingFieldOptions = MakeShared<TArray<TSharedPtr<FString>>>();
		if (GroupStruct)
		{
			TArray<TSharedPtr<FString>> AllFieldPaths;
			CollectStructFieldOptionsLocal(GroupStruct, AllFieldPaths);
			TSet<FName> ExistingFields;
			for (const int32 MappingIndex : *Group)
			{
				if (Source->InlineMappings.IsValidIndex(MappingIndex))
				{
					ExistingFields.Add(YIGetResolvedTargetFieldName(Source->InlineMappings[MappingIndex]));
				}
			}
			for (const TSharedPtr<FString>& FieldPathPtr : AllFieldPaths)
			{
				if (!FieldPathPtr.IsValid())
				{
					continue;
				}
				const FString FieldPath = *FieldPathPtr;
				FProperty* FieldProp = FindPropertyByAuthoredPathLocal(GroupStruct, FieldPath);
				if (!FieldProp || !IsMappableFragmentFieldLocal(FieldProp))
				{
					continue;
				}
				const FName FieldName(*FieldPath);
				if (!ExistingFields.Contains(FieldName))
				{
					MissingFieldOptions->Add(MakeShared<FString>(FieldPath));
				}
			}
			MissingFieldOptions->Sort([](const TSharedPtr<FString>& A, const TSharedPtr<FString>& B)
				{
					if (!A.IsValid() || !B.IsValid())
					{
						return A.IsValid();
					}
					return *A < *B;
				});
		}

		if (GroupStruct)
		{
			GroupContent->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.7f).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(MissingFieldOptions.Get())
						.IsEnabled_Lambda([MissingFieldOptions]()
							{
								return MissingFieldOptions->Num() > 0;
							})
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
							{
								return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
							})
						.OnSelectionChanged_Lambda([this, GroupKey](TSharedPtr<FString> NewItem, ESelectInfo::Type)
							{
								if (NewItem.IsValid())
								{
									PendingAddFieldByFragmentPath.Add(GroupKey, *NewItem);
								}
							})
						.Content()
						[
							SNew(STextBlock).Text_Lambda([this, GroupKey, MissingFieldOptions]()
								{
									if (MissingFieldOptions->Num() == 0)
									{
										return NSLOCTEXT("YOLOInventory", "SourceDetails_AllFieldsMapped", "All fields mapped");
									}
									if (const FString* Pending = PendingAddFieldByFragmentPath.Find(GroupKey))
									{
										return FText::FromString(*Pending);
									}
									if ((*MissingFieldOptions)[0].IsValid())
									{
										return FText::FromString(**(*MissingFieldOptions)[0]);
									}
									return NSLOCTEXT("YOLOInventory", "SourceDetails_SelectFieldToAdd", "Select field to add");
								})
						]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "SourceDetails_AddFieldButton", "Add Field"))
						.IsEnabled_Lambda([MissingFieldOptions]()
							{
								return MissingFieldOptions->Num() > 0;
							})
						.OnClicked_Lambda([this, GroupKey, GroupStruct, MissingFieldOptions]()
							{
								UYIDataTableItemSource* MutableSource = EditedSource.Get();
								if (!MutableSource || !GroupStruct || MissingFieldOptions->Num() == 0)
								{
									return FReply::Handled();
								}

								FString FieldToAdd;
								if (const FString* Pending = PendingAddFieldByFragmentPath.Find(GroupKey))
								{
									FieldToAdd = *Pending;
								}
								if (FieldToAdd.IsEmpty() && (*MissingFieldOptions)[0].IsValid())
								{
									FieldToAdd = **(*MissingFieldOptions)[0];
								}
								if (FieldToAdd.IsEmpty())
								{
									return FReply::Handled();
								}

								FProperty* TargetProp = FindPropertyByAuthoredPathLocal(GroupStruct, FieldToAdd);
								if (!TargetProp || !IsMappableFragmentFieldLocal(TargetProp))
								{
									return FReply::Handled();
								}

								MutableSource->Modify();
								FYIFieldMapping NewMapping;
								NewMapping.TargetLayer = EYIFieldMappingTargetLayer::StaticDefinitionFragment;
								NewMapping.TargetFragmentStruct = GroupStruct;
								NewMapping.TargetFragmentField = FName(*FieldToAdd);
								NewMapping.TargetProperty = NewMapping.TargetFragmentField;

								if (UDataTable* Table = MutableSource->DataTable.LoadSynchronous())
								{
									if (const UScriptStruct* RowStruct = Table->RowStruct)
									{
										if (const FProperty* SourcePathProp = FindPropertyByAuthoredPathLocal(RowStruct, FieldToAdd))
										{
											NewMapping.SourceField = FName(*FieldToAdd);
											NewMapping.Conversion = GuessConversionForPropsLocal(SourcePathProp, TargetProp);
										}
										else
										{
											const FName LeafName(*TargetProp->GetAuthoredName());
											if (const FProperty* SourceLeafProp = FindPropertyByAuthoredNameLocal(RowStruct, LeafName))
											{
												NewMapping.SourceField = LeafName;
												NewMapping.Conversion = GuessConversionForPropsLocal(SourceLeafProp, TargetProp);
											}
										}
									}
								}

								MutableSource->InlineMappings.Add(NewMapping);
								PendingAddFieldByFragmentPath.Remove(GroupKey);
								RequestRefresh();
								return FReply::Handled();
							})
				]
			];
		}

		for (const int32 MappingIndex : *Group)
		{
			GroupContent->AddSlot().AutoHeight().Padding(0.f, 3.f)
			[
				BuildMappingRowWidget(MappingIndex)
			];
		}

		Root->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
		[
			SNew(SExpandableArea)
				.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryTop"))
				.HeaderContent()
				[
					SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%s (%d)"), *GroupLabel, Group->Num())))
						.Font(IDetailLayoutBuilder::GetDetailFontBold())
				]
				.BodyContent()
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						.Padding(6.f)
						[
							GroupContent
						]
				]
		];
	}

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			Root
		];
}

TSharedRef<SWidget> FYIDataTableItemSourceDetails::BuildMappingRowWidget(int32 MappingIndex)
{
	TSharedPtr<TArray<TSharedPtr<FString>>> StaticEnumOptions = MakeShared<TArray<TSharedPtr<FString>>>();
	TSharedPtr<TArray<TSharedPtr<FString>>> StaticGameplayTagOptions = MakeShared<TArray<TSharedPtr<FString>>>();

	auto GetMapping = [this, MappingIndex]() -> FYIFieldMapping*
	{
		UYIDataTableItemSource* Source = EditedSource.Get();
		return (Source && Source->InlineMappings.IsValidIndex(MappingIndex)) ? &Source->InlineMappings[MappingIndex] : nullptr;
	};
	auto GetMappingConst = [this, MappingIndex]() -> const FYIFieldMapping*
	{
		const UYIDataTableItemSource* Source = EditedSource.Get();
		return (Source && Source->InlineMappings.IsValidIndex(MappingIndex)) ? &Source->InlineMappings[MappingIndex] : nullptr;
	};
	auto GetTargetProp = [GetMappingConst]() -> FProperty*
	{
		const FYIFieldMapping* Mapping = GetMappingConst();
		if (!Mapping || !Mapping->TargetFragmentStruct)
		{
			return nullptr;
		}
		return FindPropertyByAuthoredPathLocal(Mapping->TargetFragmentStruct.Get(), YIGetResolvedTargetFieldName(*Mapping).ToString());
	};
	auto GetSourceProp = [this, GetMappingConst]() -> const FProperty*
	{
		const FYIFieldMapping* Mapping = GetMappingConst();
		if (!Mapping || Mapping->SourceField.IsNone())
		{
			return nullptr;
		}
		const UYIDataTableItemSource* Source = EditedSource.Get();
		if (!Source)
		{
			return nullptr;
		}
		if (UDataTable* Table = Source->DataTable.LoadSynchronous())
		{
			return FindPropertyByAuthoredPathLocal(Table->RowStruct, Mapping->SourceField.ToString());
		}
		return nullptr;
	};
	auto RefreshEnumOptions = [StaticEnumOptions, GetTargetProp]()
	{
		StaticEnumOptions->Reset();
		FProperty* TargetProp = GetTargetProp();
		const UEnum* Enum = nullptr;
		if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(TargetProp))
		{
			Enum = EnumProp->GetEnum();
		}
		else if (const FNumericProperty* Num = CastField<FNumericProperty>(TargetProp))
		{
			if (Num->IsEnum())
			{
				Enum = Num->GetIntPropertyEnum();
			}
		}
		if (!Enum)
		{
			return;
		}

		for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
		{
			if (!Enum->HasMetaData(TEXT("Hidden"), Index))
			{
				StaticEnumOptions->Add(MakeShared<FString>(Enum->GetNameStringByIndex(Index)));
			}
		}
	};
	auto RefreshGameplayTagOptions = [StaticGameplayTagOptions]()
	{
		StaticGameplayTagOptions->Reset();
		FGameplayTagContainer AllTags;
		UGameplayTagsManager::Get().RequestAllGameplayTags(AllTags, true);
		TArray<FGameplayTag> Tags;
		AllTags.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
		for (const FGameplayTag& Tag : Tags)
		{
			StaticGameplayTagOptions->Add(MakeShared<FString>(Tag.ToString()));
		}
	};

	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(6.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SHyperlink)
						.Style(&FAppStyle::Get().GetWidgetStyle<FHyperlinkStyle>("Common.GotoNativeCodeHyperlink"))
						.Text_Lambda([GetMappingConst]()
							{
								const FYIFieldMapping* Mapping = GetMappingConst();
								if (!Mapping)
								{
									return FText::GetEmpty();
								}
								const FString Fragment = MakeReadableFragmentNameLocal(Mapping->TargetFragmentStruct.Get());
								return FText::FromString(FString::Printf(TEXT("%s.%s"), *Fragment, *YIGetResolvedTargetFieldName(*Mapping).ToString()));
							})
						.ToolTipText_Lambda([GetMappingConst, GetTargetProp]()
							{
								return BuildInlineMappingTargetFieldTooltipLocal(GetMappingConst(), GetTargetProp());
							})
						.OnNavigate_Lambda([GetTargetProp, GetMappingConst]()
							{
								if (const FProperty* TargetProp = GetTargetProp())
								{
									if (FSourceCodeNavigation::CanNavigateToProperty(TargetProp))
									{
										FSourceCodeNavigation::NavigateToProperty(const_cast<FProperty*>(TargetProp));
										return;
									}
								}

								if (const FYIFieldMapping* Mapping = GetMappingConst())
								{
									if (const UScriptStruct* FragmentStruct = Mapping->TargetFragmentStruct.Get())
									{
										FSourceCodeNavigation::NavigateToStruct(const_cast<UScriptStruct*>(FragmentStruct));
									}
								}
							})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(FMargin(3.f, 1.f))
						.BorderBackgroundColor_Lambda([GetTargetProp]()
							{
								FString TypeName;
								FLinearColor TypeColor;
								GetPropertyTypeInfoLocal(GetTargetProp(), TypeName, TypeColor);
								return TypeColor;
							})
						[
							SNew(STextBlock)
								.Text_Lambda([GetTargetProp]()
									{
										FString TypeName;
										FLinearColor TypeColor;
										GetPropertyTypeInfoLocal(GetTargetProp(), TypeName, TypeColor);
										return FText::FromString(TypeName);
									})
								.ColorAndOpacity(FSlateColor(FLinearColor::Black))
						]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f, 0.f, 0.f).VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([GetMappingConst]()
							{
								const FYIFieldMapping* Mapping = GetMappingConst();
								return (Mapping && Mapping->bUseStaticValue) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
							})
						.OnCheckStateChanged_Lambda([this, GetMapping](ECheckBoxState State)
							{
								if (UYIDataTableItemSource* Source = EditedSource.Get())
								{
									if (FYIFieldMapping* Mapping = GetMapping())
									{
										Source->Modify();
										Mapping->bUseStaticValue = (State == ECheckBoxState::Checked);
									}
								}
							})
						[
							SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "SourceDetails_StaticToggle", "Static"))
						]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
				[
					SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "SourceDetails_RemoveMap", "X"))
						.ButtonColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.2f, 0.2f)))
						.OnClicked_Lambda([this, MappingIndex]()
							{
								if (UYIDataTableItemSource* Source = EditedSource.Get())
								{
									if (Source->InlineMappings.IsValidIndex(MappingIndex))
									{
										Source->Modify();
										Source->InlineMappings.RemoveAt(MappingIndex, 1, EAllowShrinking::No);
										RequestRefresh();
									}
								}
								return FReply::Handled();
							})
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SWidgetSwitcher)
				.WidgetIndex_Lambda([GetMappingConst]()
					{
						const FYIFieldMapping* Mapping = GetMappingConst();
						return (Mapping && Mapping->bUseStaticValue) ? 1 : 0;
					})
				+ SWidgetSwitcher::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(0.38f).Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(SComboBox<TSharedPtr<FString>>)
							.OptionsSource(&SourceFieldOptions)
							.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
								{
									return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
								})
							.OnSelectionChanged_Lambda([this, GetMapping, GetSourceProp, GetTargetProp](TSharedPtr<FString> NewItem, ESelectInfo::Type)
								{
									if (UYIDataTableItemSource* Source = EditedSource.Get())
									{
										if (FYIFieldMapping* Mapping = GetMapping())
										{
											Source->Modify();
											Mapping->SourceField = (NewItem.IsValid() && *NewItem != TEXT("<None>")) ? FName(**NewItem) : NAME_None;
											Mapping->Conversion = GuessConversionForPropsLocal(GetSourceProp(), GetTargetProp());
										}
									}
								})
							.Content()
							[
								SNew(STextBlock).Text_Lambda([GetMappingConst]()
									{
										const FYIFieldMapping* Mapping = GetMappingConst();
										return (!Mapping || Mapping->SourceField.IsNone())
											? FText::FromString(TEXT("<None>"))
											: FText::FromName(Mapping->SourceField);
									})
							]
					]
					+ SHorizontalBox::Slot().FillWidth(0.24f).Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(SComboBox<TSharedPtr<FString>>)
							.OptionsSource(&ConversionOptions)
							.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
								{
									return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
								})
							.OnSelectionChanged_Lambda([this, GetMapping](TSharedPtr<FString> NewItem, ESelectInfo::Type)
								{
									if (UYIDataTableItemSource* Source = EditedSource.Get())
									{
										if (FYIFieldMapping* Mapping = GetMapping())
										{
											Source->Modify();
											Mapping->Conversion = NewItem.IsValid() ? LabelToConversionLocal(*NewItem) : EYIFieldMappingConversion::None;
										}
									}
								})
							.Content()
							[
								SNew(STextBlock).Text_Lambda([GetMappingConst]()
									{
										const FYIFieldMapping* Mapping = GetMappingConst();
										return FText::FromString(Mapping ? ConversionToLabelLocal(Mapping->Conversion) : TEXT("None"));
									})
							]
					]
					+ SHorizontalBox::Slot().FillWidth(0.38f)
					[
						SNew(SComboBox<TSharedPtr<FYITransformFunctionInfo>>)
							.OptionsSource(&TransformFunctionOptions)
							.OnGenerateWidget_Lambda([](TSharedPtr<FYITransformFunctionInfo> InItem)
								{
									return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? InItem->DisplayName : FString(TEXT("None"))));
								})
							.OnSelectionChanged_Lambda([this, GetMapping](TSharedPtr<FYITransformFunctionInfo> NewItem, ESelectInfo::Type)
								{
									if (UYIDataTableItemSource* Source = EditedSource.Get())
									{
										if (FYIFieldMapping* Mapping = GetMapping())
										{
											Source->Modify();
											Mapping->TransformFunction = NewItem.IsValid() ? NewItem->FunctionName : NAME_None;
											Mapping->TransformLibrary = NewItem.IsValid() ? NewItem->Library : TSoftClassPtr<UBlueprintFunctionLibrary>();
										}
									}
								})
							.Content()
							[
								SNew(STextBlock).Text_Lambda([GetMappingConst]()
									{
										const FYIFieldMapping* Mapping = GetMappingConst();
										if (!Mapping || Mapping->TransformFunction.IsNone())
										{
											return FText::FromString(TEXT("None"));
										}
										return FText::FromName(Mapping->TransformFunction);
									})
							]
					]
				]
				+ SWidgetSwitcher::Slot()
				[
					SNew(SWidgetSwitcher)
					.WidgetIndex_Lambda([GetTargetProp]()
						{
							const FProperty* TargetProp = GetTargetProp();
							if (CastField<FBoolProperty>(TargetProp)) return 0;
							if (CastField<FNumericProperty>(TargetProp)) return 1;
							if (CastField<FEnumProperty>(TargetProp)) return 2;
							if (const FStructProperty* StructProp = CastField<FStructProperty>(TargetProp))
							{
								if (StructProp->Struct == FGameplayTag::StaticStruct()) return 3;
							}
							return 4;
						})
					+ SWidgetSwitcher::Slot()
					[
						SNew(SCheckBox)
							.IsChecked_Lambda([GetMappingConst]()
								{
									const FYIFieldMapping* Mapping = GetMappingConst();
									const FString Lower = Mapping ? Mapping->StaticValue.ToLower() : FString();
									const bool bTrue = (Lower == TEXT("1") || Lower == TEXT("true") || Lower == TEXT("yes"));
									return bTrue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
								})
							.OnCheckStateChanged_Lambda([this, GetMapping](ECheckBoxState State)
								{
									if (UYIDataTableItemSource* Source = EditedSource.Get())
									{
										if (FYIFieldMapping* Mapping = GetMapping())
										{
											Source->Modify();
											Mapping->StaticValue = (State == ECheckBoxState::Checked) ? TEXT("true") : TEXT("false");
											Mapping->bUseStaticValue = true;
										}
									}
								})
							[
								SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "SourceDetails_StaticBool", "Value"))
							]
					]
					+ SWidgetSwitcher::Slot()
					[
						SNew(SNumericEntryBox<double>)
							.Value_Lambda([GetMappingConst]()
								{
									double Num = 0.0;
									const FYIFieldMapping* Mapping = GetMappingConst();
									if (Mapping && LexTryParseString(Num, *Mapping->StaticValue))
									{
										return TOptional<double>(Num);
									}
									return TOptional<double>();
								})
							.OnValueCommitted_Lambda([this, GetMapping, GetTargetProp](double NewValue, ETextCommit::Type)
								{
									if (UYIDataTableItemSource* Source = EditedSource.Get())
									{
										if (FYIFieldMapping* Mapping = GetMapping())
										{
											Source->Modify();
											if (const FNumericProperty* Num = CastField<FNumericProperty>(GetTargetProp()))
											{
												Mapping->StaticValue = Num->IsInteger()
													? LexToString((int64)FMath::RoundToInt64(NewValue))
													: LexToString(NewValue);
											}
											else
											{
												Mapping->StaticValue = LexToString(NewValue);
											}
											Mapping->bUseStaticValue = true;
										}
									}
								})
					]
					+ SWidgetSwitcher::Slot()
					[
						SNew(SComboBox<TSharedPtr<FString>>)
							.OptionsSource(StaticEnumOptions.Get())
							.OnComboBoxOpening_Lambda([RefreshEnumOptions]()
								{
									RefreshEnumOptions();
								})
							.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
								{
									return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
								})
							.OnSelectionChanged_Lambda([this, GetMapping](TSharedPtr<FString> NewItem, ESelectInfo::Type)
								{
									if (UYIDataTableItemSource* Source = EditedSource.Get())
									{
										if (FYIFieldMapping* Mapping = GetMapping())
										{
											Source->Modify();
											Mapping->StaticValue = NewItem.IsValid() ? *NewItem : FString();
											Mapping->bUseStaticValue = true;
										}
									}
								})
							.Content()
							[
								SNew(STextBlock).Text_Lambda([GetMappingConst]()
									{
										const FYIFieldMapping* Mapping = GetMappingConst();
										return FText::FromString((Mapping && !Mapping->StaticValue.IsEmpty()) ? Mapping->StaticValue : TEXT("Select enum value"));
									})
							]
					]
					+ SWidgetSwitcher::Slot()
					[
						SNew(SComboBox<TSharedPtr<FString>>)
							.OptionsSource(StaticGameplayTagOptions.Get())
							.OnComboBoxOpening_Lambda([RefreshGameplayTagOptions]()
								{
									RefreshGameplayTagOptions();
								})
							.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
								{
									return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
								})
							.OnSelectionChanged_Lambda([this, GetMapping](TSharedPtr<FString> NewItem, ESelectInfo::Type)
								{
									if (UYIDataTableItemSource* Source = EditedSource.Get())
									{
										if (FYIFieldMapping* Mapping = GetMapping())
										{
											Source->Modify();
											Mapping->StaticValue = NewItem.IsValid() ? *NewItem : FString();
											Mapping->bUseStaticValue = true;
										}
									}
								})
							.Content()
							[
								SNew(STextBlock).Text_Lambda([GetMappingConst]()
									{
										const FYIFieldMapping* Mapping = GetMappingConst();
										return FText::FromString((Mapping && !Mapping->StaticValue.IsEmpty()) ? Mapping->StaticValue : TEXT("Select gameplay tag"));
									})
							]
					]
					+ SWidgetSwitcher::Slot()
					[
						SNew(SEditableTextBox)
							.Text_Lambda([GetMappingConst]()
								{
									const FYIFieldMapping* Mapping = GetMappingConst();
									return FText::FromString(Mapping ? Mapping->StaticValue : FString());
								})
							.OnTextCommitted_Lambda([this, GetMapping](const FText& NewText, ETextCommit::Type)
								{
									if (UYIDataTableItemSource* Source = EditedSource.Get())
									{
										if (FYIFieldMapping* Mapping = GetMapping())
										{
											Source->Modify();
											Mapping->StaticValue = NewText.ToString();
											Mapping->bUseStaticValue = true;
										}
									}
								})
					]
				]
			]
		];
}
