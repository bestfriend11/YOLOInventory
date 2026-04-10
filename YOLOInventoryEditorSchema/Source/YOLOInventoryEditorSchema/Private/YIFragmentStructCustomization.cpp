#include "YIFragmentStructCustomization.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "SourceCodeNavigation.h"
#include "Styling/AppStyle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	static FString YIFragmentStructReadableName(const UScriptStruct* StructType)
	{
		if (!StructType)
		{
			return TEXT("Fragment");
		}

		FString Name = StructType->GetMetaData(TEXT("DisplayName"));
		if (Name.IsEmpty())
		{
			Name = StructType->GetName();
		}

		Name.RemoveFromStart(TEXT("FYI"));
		Name.RemoveFromStart(TEXT("YI"));
		Name.RemoveFromStart(TEXT("Item"));
		Name.RemoveFromEnd(TEXT("DefinitionFragment"));
		Name.RemoveFromEnd(TEXT("Fragment"));
		return Name.TrimStartAndEnd();
	}

	static FString YIGetPropertyTypeHint(const FProperty* Property)
	{
		if (!Property)
		{
			return TEXT("Unknown");
		}
		if (CastField<FBoolProperty>(Property)) return TEXT("Bool");
		if (CastField<FIntProperty>(Property)) return TEXT("Int32");
		if (CastField<FInt64Property>(Property)) return TEXT("Int64");
		if (CastField<FFloatProperty>(Property)) return TEXT("Float");
		if (CastField<FDoubleProperty>(Property)) return TEXT("Double");
		if (CastField<FNameProperty>(Property)) return TEXT("Name");
		if (CastField<FStrProperty>(Property)) return TEXT("String");
		if (CastField<FTextProperty>(Property)) return TEXT("Text");
		if (CastField<FStructProperty>(Property)) return TEXT("Struct");
		if (CastField<FObjectPropertyBase>(Property)) return TEXT("Object Ref");
		if (CastField<FArrayProperty>(Property)) return TEXT("Array");
		if (CastField<FMapProperty>(Property)) return TEXT("Map");
		return Property->GetClass() ? Property->GetClass()->GetName() : TEXT("Property");
	}

	static bool YITryGetFieldGuidance(const FProperty* Property, FString& OutGuidance)
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

		// Generic fragment authoring rules
		if (FieldName == TEXT("FragmentTag"))
		{
			return Set(TEXT("Semantic lookup key used by gameplay/runtime systems.\nBind this when your system resolves fragments by GameplayTag.\nSafe to leave empty if the fragment is only consumed by direct struct lookup."));
		}
		if (FieldName == TEXT("FragmentName"))
		{
			return Set(TEXT("Human-readable secondary key for designers/debugging.\nUseful for multiple custom fragments with the same tag or for dashboard readability.\nUsually optional."));
		}
		if (FieldName == TEXT("Properties"))
		{
			return Set(TEXT("Custom data payload (PropertyBag).\nUse this for no-C++ authoring. Add typed fields here, then bind CSV inline mappings to Properties.<FieldName>.\nOnly fields used by your runtime systems need to be populated."));
		}

		// Common item schema fragments
		if (OwnerName == TEXT("YIItemUIDefinitionFragment"))
		{
			if (FieldName == TEXT("DisplayName")) return Set(TEXT("Primary item name shown in dashboards, tooltips, and inventory UI.\nBind this for almost every player-facing item.\nIf omitted, tools may fall back to asset name (less useful for designers)."));
			if (FieldName == TEXT("Description")) return Set(TEXT("Long-form description / flavor text.\nBind if your game shows inspect/tooltips; optional otherwise."));
			if (FieldName == TEXT("Icon")) return Set(TEXT("UI icon soft reference.\nBind if the item appears in inventory, shops, or tooltips with images."));
		}
		if (OwnerName == TEXT("YIItemClassificationDefinitionFragment"))
		{
			if (FieldName == TEXT("ItemType")) return Set(TEXT("Primary gameplay classification tag (e.g. Item.Weapon.Sword).\nBind this when systems branch on item class or slot logic."));
			if (FieldName == TEXT("Tags")) return Set(TEXT("Secondary tags for filtering/rules/UI.\nBind only tags your game actually uses; keep minimal to avoid noisy data."));
			if (FieldName == TEXT("RarityTag")) return Set(TEXT("Rarity classification tag.\nBind when rarity affects UI color, drop logic, economy, or progression."));
		}
		if (OwnerName == TEXT("YIItemLayoutDefinitionFragment"))
		{
			if (FieldName == TEXT("DefaultSize")) return Set(TEXT("Grid footprint (width x height).\nBind for grid inventories. Ignore for list/Skyrim-style inventories."));
			if (FieldName == TEXT("bAllowRotation")) return Set(TEXT("Whether the item can rotate in grid placement.\nOnly relevant for grid inventory UIs with rotation support."));
		}
		if (OwnerName == TEXT("YIItemEquipmentDefinitionFragment"))
		{
			if (FieldName == TEXT("PrimaryEquipSlot")) return Set(TEXT("Preferred slot tag when auto-equipping.\nBind if the item is equipable."));
			if (FieldName == TEXT("OccupiedSlots")) return Set(TEXT("All slots occupied when equipped (for multi-slot items / two-handers / designer-defined layouts).\nBind only for equipable items that consume multiple slots or explicit slot sets."));
		}
		if (OwnerName == TEXT("YIItemStackingDefinitionFragment"))
		{
			if (FieldName == TEXT("MaxStackCount")) return Set(TEXT("Maximum items per stack.\nIf the stacking fragment is absent, the item is non-stackable."));
			if (FieldName == TEXT("bUseRiskChecks")) return Set(TEXT("Enables runtime safety checks that prevent unsafe stacking of mutable/randomized items.\nKeep enabled unless you intentionally override safety rules."));
		}
		if (OwnerName == TEXT("YIItemRulesDefinitionFragment"))
		{
			if (FieldName == TEXT("bUniquePerType")) return Set(TEXT("Bag-level uniqueness by item type/definition.\nUse for quest items, unique keys, or restricted equipment archetypes."));
			if (FieldName == TEXT("EquipSlotCost")) return Set(TEXT("Capacity cost consumed in the target equipment slot schema.\nUse when slot capacity matters (e.g. chest armor cost tiers)."));
		}
		if (OwnerName == TEXT("YIItemContainerDefinitionFragment"))
		{
			if (FieldName == TEXT("bIsContainerItem")) return Set(TEXT("Marks item as a bag/container (bag-in-bag capable item).\nRequired for nested container runtime logic."));
			if (FieldName == TEXT("ContainerTemplateBag")) return Set(TEXT("Optional template bag asset for initial nested layout/content.\nBind when you need a predefined container arrangement."));
			if (FieldName == TEXT("ContainerDefaultGridSize")) return Set(TEXT("Fallback nested bag size when no template bag is provided.\nIgnored when a template bag is assigned."));
		}
		if (OwnerName == TEXT("YIItemEquipRequirementsFragment"))
		{
			if (FieldName == TEXT("MinLevel")) return Set(TEXT("Minimum character/account level to equip this item.\nAuthor only when your game enforces level gates."));
			if (FieldName == TEXT("RequiredTags")) return Set(TEXT("All required tags that must exist on the equipping actor/state.\nUse for class/faction/progression gates."));
			if (FieldName == TEXT("BlockedTags")) return Set(TEXT("Tags that forbid equipping while present on the actor/state.\nUse for anti-state restrictions (stunned, shapeshifted, etc)."));
		}
		if (OwnerName == TEXT("YIItemTradePolicyFragment"))
		{
			if (FieldName == TEXT("bTradable")) return Set(TEXT("Master switch for whether this item can be traded at all.\nServer-side trade validation should always enforce this."));
			if (FieldName == TEXT("bVisibleInTrade")) return Set(TEXT("Controls whether item appears in trade UIs/offer pickers.\nUseful for hidden/system items."));
			if (FieldName == TEXT("RequiredTradeTags")) return Set(TEXT("Trade context tags required for this item to be offered.\nUse for event/season/vendor-specific trade rules."));
			if (FieldName == TEXT("BlockedTradeTags")) return Set(TEXT("Trade context tags that block this item from being offered.\nUse for restricted zones/modes/rulesets."));
		}
		if (OwnerName == TEXT("YIItemAffixDefinitionFragment"))
		{
			if (FieldName == TEXT("TemplateAffixes")) return Set(TEXT("Legacy affix asset references always applied to new instances.\nLegacy path; prefer fragment-based roll strategies for new systems."));
			if (FieldName == TEXT("MinRandomModifiers") || FieldName == TEXT("MaxRandomModifiers")) return Set(TEXT("Legacy randomized affix count range.\nLegacy generator path only; ignore if using fragment roll strategies."));
			if (FieldName == TEXT("PrefixPool") || FieldName == TEXT("SuffixPool")) return Set(TEXT("Legacy affix pools used by legacy generator path.\nPrefer fragment pools for new content pipelines."));
		}
		if (OwnerName == TEXT("YIItemAffixesFragment"))
		{
			if (FieldName == TEXT("Values")) return Set(TEXT("Legacy runtime affix payload array.\nKept for compatibility; new gameplay systems should move to custom/typed runtime fragments."));
		}
		if (OwnerName == TEXT("YIItemAttributesFragment"))
		{
			if (FieldName == TEXT("Values")) return Set(TEXT("Generic runtime numeric key/value attributes.\nUse only if you intentionally rely on this compatibility fragment; custom runtime fragments are usually clearer."));
		}
		if (OwnerName == TEXT("YIItemDurabilityFragment"))
		{
			if (FieldName == TEXT("bEnabled")) return Set(TEXT("Enables durability state for this instance.\nUse with Current/Max values in runtime pipelines."));
			if (FieldName == TEXT("Current")) return Set(TEXT("Current durability value.\nMutable runtime state; should not be authored as shared static data."));
			if (FieldName == TEXT("Max")) return Set(TEXT("Maximum durability value.\nCan be initialized from generation/equip logic or copied from a definition/runtime template."));
		}
		if (OwnerName == TEXT("YIItemDurabilityRuntimeFragment"))
		{
			if (FieldName == TEXT("bEnabled")) return Set(TEXT("Enables durability runtime state for this item instance."));
			if (FieldName == TEXT("Current")) return Set(TEXT("Current durability (integer).\nReplicate/mutate this value at runtime when the item takes wear."));
			if (FieldName == TEXT("Max")) return Set(TEXT("Maximum durability (integer).\nSet from generation/equip initialization rules."));
		}
		if (OwnerName == TEXT("YIItemChargesRuntimeFragment"))
		{
			if (FieldName == TEXT("Current")) return Set(TEXT("Current charge count for this item instance.\nDecrement on use, refill via gameplay systems."));
			if (FieldName == TEXT("Max")) return Set(TEXT("Maximum charge count for this item instance."));
		}
		if (OwnerName == TEXT("YIItemCooldownRuntimeFragment"))
		{
			if (FieldName == TEXT("LastActivatedServerTime")) return Set(TEXT("Authoritative server timestamp of last successful activation.\nUse this with CooldownDurationSeconds to compute remaining cooldown."));
			if (FieldName == TEXT("CooldownDurationSeconds")) return Set(TEXT("Cooldown duration in seconds.\nCan be scaled by level/quality in runtime systems before use."));
		}
		if (OwnerName == TEXT("YIItemBindStateRuntimeFragment"))
		{
			if (FieldName == TEXT("bAccountBound")) return Set(TEXT("When true, item cannot leave owning account through trade/transfer."));
			if (FieldName == TEXT("bCharacterBound")) return Set(TEXT("When true, item cannot leave owning character through trade/transfer."));
			if (FieldName == TEXT("TradeLockedUntilServerTime")) return Set(TEXT("Server timestamp until which trade is locked.\n<= 0 means no timer lock."));
		}

		return false;
	}

	static FText YIBuildTooltipFromProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return FText::GetEmpty();
		}

		const FString ExistingTooltip = Property->GetToolTipText().ToString();
		const FString TypeHint = YIGetPropertyTypeHint(Property);

		FString Guidance;
		YITryGetFieldGuidance(Property, Guidance);

		FString Combined;
		if (!ExistingTooltip.IsEmpty())
		{
			Combined += ExistingTooltip;
		}
		if (!TypeHint.IsEmpty())
		{
			if (!Combined.IsEmpty()) Combined += TEXT("\n\n");
			Combined += FString::Printf(TEXT("Type: %s"), *TypeHint);
		}
		if (!Guidance.IsEmpty())
		{
			if (!Combined.IsEmpty()) Combined += TEXT("\n\n");
			Combined += TEXT("Usage / Binding Guidance:\n");
			Combined += Guidance;
		}
		if (!Combined.IsEmpty())
		{
			Combined += TEXT("\n\nClick field title to open native source.");
		}
		return FText::FromString(Combined);
	}

	static FText YIBuildTooltipFromStruct(const UScriptStruct* StructType)
	{
		if (!StructType)
		{
			return FText::GetEmpty();
		}

		FString Tooltip = StructType->GetToolTipText().ToString();
		if (Tooltip.IsEmpty())
		{
			Tooltip = FString::Printf(TEXT("%s fragment."), *YIFragmentStructReadableName(StructType));
		}
		Tooltip += TEXT("\n\nClick fragment title to open native source.");
		return FText::FromString(Tooltip);
	}
}

TSharedRef<IPropertyTypeCustomization> FYIFragmentStructCustomization::MakeInstance()
{
	return MakeShared<FYIFragmentStructCustomization>();
}

void FYIFragmentStructCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	(void)StructCustomizationUtils;

	HeaderRow
	.NameContent()
	[
		MakeClickableStructLabelWidget(StructPropertyHandle)
	]
	.ValueContent()
	.MinDesiredWidth(250.f)
	.MaxDesiredWidth(700.f)
	[
		StructPropertyHandle->CreatePropertyValueWidget()
	];
}

void FYIFragmentStructCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	(void)StructCustomizationUtils;
	(void)StructPropertyHandle;

	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);
		if (!ChildHandle.IsValid() || !ChildHandle->IsValidHandle())
		{
			continue;
		}

		IDetailPropertyRow& Row = StructBuilder.AddProperty(ChildHandle.ToSharedRef());
		TSharedPtr<SWidget> DefaultNameWidget;
		TSharedPtr<SWidget> DefaultValueWidget;
		Row.GetDefaultWidgets(DefaultNameWidget, DefaultValueWidget, false);

		Row.CustomWidget(false);
		if (FDetailWidgetDecl* NameWidgetDecl = Row.CustomNameWidget())
		{
			NameWidgetDecl
				->MinDesiredWidth(220.f)
				.MaxDesiredWidth(420.f)
			[
				MakeClickableFieldLabelWidget(ChildHandle.ToSharedRef())
			];
		}

		if (FDetailWidgetDecl* ValueWidgetDecl = Row.CustomValueWidget())
		{
			ValueWidgetDecl
				->MinDesiredWidth(280.f)
				.MaxDesiredWidth(800.f)
			[
				DefaultValueWidget.IsValid()
					? DefaultValueWidget.ToSharedRef()
					: ChildHandle->CreatePropertyValueWidget(true)
			];
		}
	}
}

TSharedRef<SWidget> FYIFragmentStructCustomization::MakeClickableStructLabelWidget(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	const FProperty* Property = StructPropertyHandle->GetProperty();
	const FStructProperty* StructProp = CastField<FStructProperty>(Property);
	const UScriptStruct* StructType = StructProp ? StructProp->Struct.Get() : nullptr;

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(SHyperlink)
			.Style(&FAppStyle::Get().GetWidgetStyle<FHyperlinkStyle>("Common.GotoNativeCodeHyperlink"))
			.Text_Lambda([StructPropertyHandle]()
			{
				return StructPropertyHandle->GetPropertyDisplayName();
			})
			.ToolTipText(BuildStructTooltip(StructType))
			.OnNavigate_Lambda([StructType]()
			{
				NavigateToStructSource(StructType);
			})
		];
}

TSharedRef<SWidget> FYIFragmentStructCustomization::MakeClickableFieldLabelWidget(TSharedRef<IPropertyHandle> PropertyHandle)
{
	const FProperty* Property = PropertyHandle->GetProperty();

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f)
		.VAlign(VAlign_Center)
		[
			SNew(SHyperlink)
			.Style(&FAppStyle::Get().GetWidgetStyle<FHyperlinkStyle>("Common.GotoNativeCodeHyperlink"))
			.Text(GetReadableFieldLabel(PropertyHandle))
			.ToolTipText(BuildFieldTooltip(Property))
			.OnNavigate_Lambda([Property]()
			{
				NavigateToPropertySource(Property);
			})
		];
}

FText FYIFragmentStructCustomization::BuildStructTooltip(const UScriptStruct* StructType)
{
	return YIBuildTooltipFromStruct(StructType);
}

FText FYIFragmentStructCustomization::BuildFieldTooltip(const FProperty* Property)
{
	return YIBuildTooltipFromProperty(Property);
}

FText FYIFragmentStructCustomization::GetReadableFieldLabel(TSharedRef<IPropertyHandle> PropertyHandle)
{
	return PropertyHandle->GetPropertyDisplayName();
}

FReply FYIFragmentStructCustomization::NavigateToPropertySource(const FProperty* Property)
{
	if (Property)
	{
		if (FSourceCodeNavigation::CanNavigateToProperty(Property))
		{
			FSourceCodeNavigation::NavigateToProperty(const_cast<FProperty*>(Property));
		}
		else if (const UStruct* OwnerStruct = Property->GetOwnerStruct())
		{
			FSourceCodeNavigation::NavigateToStruct(const_cast<UStruct*>(OwnerStruct));
		}
	}
	return FReply::Handled();
}

FReply FYIFragmentStructCustomization::NavigateToStructSource(const UScriptStruct* StructType)
{
	if (StructType)
	{
		FSourceCodeNavigation::NavigateToStruct(const_cast<UScriptStruct*>(StructType));
	}
	return FReply::Handled();
}
