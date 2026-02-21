#include "YIDataTableItemSourceDetails.h"

#include "Data/YIDataTableItemSource.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/DataTable.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "IDetailChildrenBuilder.h"
#include "Misc/PackageName.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
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
	if (!FragmentStruct)
	{
		return true;
	}

	FInstancedStruct Tmp;
	Tmp.InitializeAs(FragmentStruct);
	if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(FindPropertyByAuthoredNameLocal(FragmentStruct, FName(TEXT("bIsUniqueFragment")))))
	{
		const uint8* Data = Tmp.GetMemory();
		return Data ? BoolProp->GetPropertyValue_InContainer(Data) : true;
	}
	return true;
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
			if (FindPropertyByAuthoredPathLocal(RowStruct, FieldPath))
			{
				NewMapping.SourceField = FName(*FieldPath);
				NewMapping.Conversion = EYIFieldMappingConversion::None;
			}
			else
			{
				const FName LeafName(*TargetProp->GetAuthoredName());
				if (FindPropertyByAuthoredNameLocal(RowStruct, LeafName))
				{
					NewMapping.SourceField = LeafName;
					NewMapping.Conversion = EYIFieldMappingConversion::None;
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

		if (FindPropertyByAuthoredPathLocal(Table->RowStruct, TargetFieldPath))
		{
			Mapping.SourceField = FName(*TargetFieldPath);
			Mapping.Conversion = EYIFieldMappingConversion::None;
			continue;
		}

		const FName LeafName(*TargetProp->GetAuthoredName());
		if (FindPropertyByAuthoredNameLocal(Table->RowStruct, LeafName))
		{
			Mapping.SourceField = LeafName;
			Mapping.Conversion = EYIFieldMappingConversion::None;
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
					SNew(STextBlock).Text_Lambda([this, MappingIndex]()
						{
							const UYIDataTableItemSource* Source = EditedSource.Get();
							if (!Source || !Source->InlineMappings.IsValidIndex(MappingIndex))
							{
								return FText::GetEmpty();
							}
							const FYIFieldMapping& Mapping = Source->InlineMappings[MappingIndex];
							const FName ResolvedTarget = YIGetResolvedTargetFieldName(Mapping);
							const FString Fragment = MakeReadableFragmentNameLocal(Mapping.TargetFragmentStruct.Get());
							return FText::FromString(FString::Printf(TEXT("%s.%s"), *Fragment, *ResolvedTarget.ToString()));
						})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(FMargin(3.f, 1.f))
						.BorderBackgroundColor_Lambda([this, MappingIndex]()
							{
								const UYIDataTableItemSource* Source = EditedSource.Get();
								if (!Source || !Source->InlineMappings.IsValidIndex(MappingIndex))
								{
									return FLinearColor(0.2f, 0.2f, 0.2f, 1.f);
								}
								const FYIFieldMapping& Mapping = Source->InlineMappings[MappingIndex];
								const FName ResolvedTarget = YIGetResolvedTargetFieldName(Mapping);
								const FProperty* TargetProp = Mapping.TargetFragmentStruct
									? FindPropertyByAuthoredPathLocal(Mapping.TargetFragmentStruct.Get(), ResolvedTarget.ToString())
									: nullptr;
								FString TypeName;
								FLinearColor TypeColor;
								GetPropertyTypeInfoLocal(TargetProp, TypeName, TypeColor);
								return TypeColor;
							})
						[
							SNew(STextBlock)
								.Text_Lambda([this, MappingIndex]()
									{
										const UYIDataTableItemSource* Source = EditedSource.Get();
										if (!Source || !Source->InlineMappings.IsValidIndex(MappingIndex))
										{
											return FText::FromString(TEXT("Any"));
										}
										const FYIFieldMapping& Mapping = Source->InlineMappings[MappingIndex];
										const FName ResolvedTarget = YIGetResolvedTargetFieldName(Mapping);
										const FProperty* TargetProp = Mapping.TargetFragmentStruct
											? FindPropertyByAuthoredPathLocal(Mapping.TargetFragmentStruct.Get(), ResolvedTarget.ToString())
											: nullptr;
										FString TypeName;
										FLinearColor TypeColor;
										GetPropertyTypeInfoLocal(TargetProp, TypeName, TypeColor);
										return FText::FromString(TypeName);
									})
								.ColorAndOpacity(FSlateColor(FLinearColor::Black))
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
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&SourceFieldOptions)
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
							{
								return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
							})
						.OnSelectionChanged_Lambda([this, MappingIndex](TSharedPtr<FString> NewItem, ESelectInfo::Type)
							{
								if (UYIDataTableItemSource* Source = EditedSource.Get())
								{
									if (Source->InlineMappings.IsValidIndex(MappingIndex) && NewItem.IsValid())
									{
										Source->Modify();
										Source->InlineMappings[MappingIndex].SourceField = (*NewItem == TEXT("<None>")) ? NAME_None : FName(**NewItem);
									}
								}
							})
						.Content()
						[
							SNew(STextBlock).Text_Lambda([this, MappingIndex]()
								{
									const UYIDataTableItemSource* Source = EditedSource.Get();
									if (!Source || !Source->InlineMappings.IsValidIndex(MappingIndex))
									{
										return FText::FromString(TEXT("<None>"));
									}
									const FName SourceField = Source->InlineMappings[MappingIndex].SourceField;
									return SourceField.IsNone()
										? FText::FromString(TEXT("<None>"))
										: FText::FromName(SourceField);
								})
						]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f).VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([this, MappingIndex]()
							{
								const UYIDataTableItemSource* Source = EditedSource.Get();
								const bool bOn = Source && Source->InlineMappings.IsValidIndex(MappingIndex)
									? Source->InlineMappings[MappingIndex].bUseStaticValue
									: false;
								return bOn ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
							})
						.OnCheckStateChanged_Lambda([this, MappingIndex](ECheckBoxState State)
							{
								if (UYIDataTableItemSource* Source = EditedSource.Get())
								{
									if (Source->InlineMappings.IsValidIndex(MappingIndex))
									{
										Source->Modify();
										Source->InlineMappings[MappingIndex].bUseStaticValue = (State == ECheckBoxState::Checked);
									}
								}
							})
						[
							SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "SourceDetails_StaticValue", "Static"))
						]
				]
				+ SHorizontalBox::Slot().FillWidth(0.30f)
				[
					SNew(SEditableTextBox)
						.Text_Lambda([this, MappingIndex]()
							{
								const UYIDataTableItemSource* Source = EditedSource.Get();
								if (!Source || !Source->InlineMappings.IsValidIndex(MappingIndex))
								{
									return FText::GetEmpty();
								}
								return FText::FromString(Source->InlineMappings[MappingIndex].StaticValue);
							})
						.OnTextCommitted_Lambda([this, MappingIndex](const FText& NewText, ETextCommit::Type)
							{
								if (UYIDataTableItemSource* Source = EditedSource.Get())
								{
									if (Source->InlineMappings.IsValidIndex(MappingIndex))
									{
										Source->Modify();
										Source->InlineMappings[MappingIndex].StaticValue = NewText.ToString();
										Source->InlineMappings[MappingIndex].bUseStaticValue = true;
									}
								}
							})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&ConversionOptions)
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
							{
								return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
							})
						.OnSelectionChanged_Lambda([this, MappingIndex](TSharedPtr<FString> NewItem, ESelectInfo::Type)
							{
								if (UYIDataTableItemSource* Source = EditedSource.Get())
								{
									if (Source->InlineMappings.IsValidIndex(MappingIndex) && NewItem.IsValid())
									{
										Source->Modify();
										Source->InlineMappings[MappingIndex].Conversion = LabelToConversionLocal(*NewItem);
									}
								}
							})
						.Content()
						[
							SNew(STextBlock).Text_Lambda([this, MappingIndex]()
								{
									const UYIDataTableItemSource* Source = EditedSource.Get();
									if (!Source || !Source->InlineMappings.IsValidIndex(MappingIndex))
									{
										return FText::FromString(TEXT("None"));
									}
									return FText::FromString(ConversionToLabelLocal(Source->InlineMappings[MappingIndex].Conversion));
								})
						]
				]
				+ SHorizontalBox::Slot().FillWidth(0.55f)
				[
					SNew(SComboBox<TSharedPtr<FYITransformFunctionInfo>>)
						.OptionsSource(&TransformFunctionOptions)
						.OnGenerateWidget_Lambda([](TSharedPtr<FYITransformFunctionInfo> InItem)
							{
								return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? InItem->DisplayName : FString(TEXT("None"))));
							})
						.OnSelectionChanged_Lambda([this, MappingIndex](TSharedPtr<FYITransformFunctionInfo> NewItem, ESelectInfo::Type)
							{
								if (UYIDataTableItemSource* Source = EditedSource.Get())
								{
									if (Source->InlineMappings.IsValidIndex(MappingIndex) && NewItem.IsValid())
									{
										Source->Modify();
										Source->InlineMappings[MappingIndex].TransformFunction = NewItem->FunctionName;
										Source->InlineMappings[MappingIndex].TransformLibrary = NewItem->Library;
									}
								}
							})
						.Content()
						[
							SNew(STextBlock).Text_Lambda([this, MappingIndex]()
								{
									const UYIDataTableItemSource* Source = EditedSource.Get();
									if (!Source || !Source->InlineMappings.IsValidIndex(MappingIndex))
									{
										return FText::FromString(TEXT("None"));
									}
									const FYIFieldMapping& Mapping = Source->InlineMappings[MappingIndex];
									if (Mapping.TransformFunction.IsNone())
									{
										return FText::FromString(TEXT("None"));
									}
									return FText::FromName(Mapping.TransformFunction);
								})
						]
				]
			]
		];
}
