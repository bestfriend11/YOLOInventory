#include "YIItemDefinitionPresetLibrary.h"

#include "GameplayTagsManager.h"
#include "YIItemDefinition.h"
#include "YIItemFragments.h"
#include "YIItemSchemaResolver.h"

namespace
{
	static FGameplayTag YI_RequestTag(const TCHAR* TagPath)
	{
		return UGameplayTagsManager::Get().RequestGameplayTag(FName(TagPath), false);
	}

	template<typename TFragmentType>
	static TFragmentType* YI_FindOrAddFragment(UYIItemDefinition* ItemDefinition)
	{
		if (!ItemDefinition)
		{
			return nullptr;
		}

		if (FInstancedStruct* FragmentStruct = ItemDefinition->FindOrAddDefinitionFragmentByStruct(TFragmentType::StaticStruct()))
		{
			return FragmentStruct->GetMutablePtr<TFragmentType>();
		}
		return nullptr;
	}

	static void YI_ApplyClassificationPreset(
		UYIItemDefinition* ItemDefinition,
		const FGameplayTag ItemTypeTag,
		const FGameplayTag RarityTag,
		const TArray<FGameplayTag>& ExtraTags,
		const bool bOverwritePresetFields)
	{
		FYIItemClassificationDefinitionFragment* Classification = YI_FindOrAddFragment<FYIItemClassificationDefinitionFragment>(ItemDefinition);
		if (!Classification)
		{
			return;
		}

		if (bOverwritePresetFields || !Classification->ItemType.IsValid())
		{
			if (ItemTypeTag.IsValid())
			{
				Classification->ItemType = ItemTypeTag;
			}
		}

		if (bOverwritePresetFields || !Classification->RarityTag.IsValid())
		{
			if (RarityTag.IsValid())
			{
				Classification->RarityTag = RarityTag;
			}
		}

		for (const FGameplayTag& ExtraTag : ExtraTags)
		{
			if (ExtraTag.IsValid() && !Classification->Tags.HasTagExact(ExtraTag))
			{
				Classification->Tags.AddTag(ExtraTag);
			}
		}
	}

	static void YI_ApplyEquipmentPreset(
		UYIItemDefinition* ItemDefinition,
		const FGameplayTag PrimarySlotTag,
		const int32 EquipSlotCost,
		const bool bOverwritePresetFields)
	{
		FYIItemEquipmentDefinitionFragment* Equipment = YI_FindOrAddFragment<FYIItemEquipmentDefinitionFragment>(ItemDefinition);
		if (Equipment)
		{
			if (bOverwritePresetFields || !Equipment->PrimaryEquipSlot.IsValid())
			{
				if (PrimarySlotTag.IsValid())
				{
					Equipment->PrimaryEquipSlot = PrimarySlotTag;
				}
			}

			if (PrimarySlotTag.IsValid() && !Equipment->OccupiedSlots.HasTagExact(PrimarySlotTag))
			{
				Equipment->OccupiedSlots.AddTag(PrimarySlotTag);
			}
		}

		FYIItemRulesDefinitionFragment* Rules = YI_FindOrAddFragment<FYIItemRulesDefinitionFragment>(ItemDefinition);
		if (Rules && (bOverwritePresetFields || Rules->EquipSlotCost <= 1))
		{
			Rules->EquipSlotCost = FMath::Max(1, EquipSlotCost);
		}
	}
}

bool UYIItemDefinitionPresetLibrary::ApplyPresetToItemDefinition(UYIItemDefinition* ItemDefinition, const EYIItemDefinitionPreset Preset, const bool bOverwritePresetFields)
{
	if (!ItemDefinition)
	{
		return false;
	}

	ItemDefinition->Modify();

	FYIItemUIDefinitionFragment* UI = YI_FindOrAddFragment<FYIItemUIDefinitionFragment>(ItemDefinition);
	if (UI && UI->DisplayName.IsEmpty())
	{
		UI->DisplayName = FText::FromString(ItemDefinition->GetName());
	}

	switch (Preset)
	{
	case EYIItemDefinitionPreset::Weapon:
		YI_ApplyClassificationPreset(
			ItemDefinition,
			YI_RequestTag(TEXT("Item.Weapon")),
			YI_RequestTag(TEXT("Rarity.Common")),
			{YI_RequestTag(TEXT("Item.Equipment"))},
			bOverwritePresetFields);
		YI_ApplyEquipmentPreset(ItemDefinition, YI_RequestTag(TEXT("Equipment.Weapon.MainHand")), 1, bOverwritePresetFields);
		break;

	case EYIItemDefinitionPreset::Armor:
		YI_ApplyClassificationPreset(
			ItemDefinition,
			YI_RequestTag(TEXT("Item.Armor")),
			YI_RequestTag(TEXT("Rarity.Common")),
			{YI_RequestTag(TEXT("Item.Equipment"))},
			bOverwritePresetFields);
		YI_ApplyEquipmentPreset(ItemDefinition, YI_RequestTag(TEXT("Equipment.Armor.Chest")), 1, bOverwritePresetFields);
		break;

	case EYIItemDefinitionPreset::Consumable:
		YI_ApplyClassificationPreset(
			ItemDefinition,
			YI_RequestTag(TEXT("Item.Consumable")),
			YI_RequestTag(TEXT("Rarity.Common")),
			{},
			bOverwritePresetFields);
		{
			FYIItemStackingDefinitionFragment* Stacking = YI_FindOrAddFragment<FYIItemStackingDefinitionFragment>(ItemDefinition);
			if (Stacking && (bOverwritePresetFields || !Stacking->bAllowStacking))
			{
				Stacking->bAllowStacking = true;
				Stacking->MaxStackCount = FMath::Max(10, Stacking->MaxStackCount);
			}
		}
		break;

	case EYIItemDefinitionPreset::Container:
		YI_ApplyClassificationPreset(
			ItemDefinition,
			YI_RequestTag(TEXT("Item.Container")),
			YI_RequestTag(TEXT("Rarity.Common")),
			{},
			bOverwritePresetFields);
		{
			FYIItemContainerDefinitionFragment* Container = YI_FindOrAddFragment<FYIItemContainerDefinitionFragment>(ItemDefinition);
			if (Container)
			{
				Container->bIsContainerItem = true;
				if (bOverwritePresetFields || Container->ContainerDefaultGridSize.X <= 0 || Container->ContainerDefaultGridSize.Y <= 0)
				{
					Container->ContainerDefaultGridSize = FIntPoint(6, 8);
				}
			}
			FYIItemStackingDefinitionFragment* Stacking = YI_FindOrAddFragment<FYIItemStackingDefinitionFragment>(ItemDefinition);
			if (Stacking)
			{
				Stacking->bAllowStacking = false;
				Stacking->MaxStackCount = 1;
				Stacking->bUseRiskChecks = true;
			}
		}
		break;

	case EYIItemDefinitionPreset::Spellbook:
		YI_ApplyClassificationPreset(
			ItemDefinition,
			YI_RequestTag(TEXT("Item.Spellbook")),
			YI_RequestTag(TEXT("Rarity.Common")),
			{YI_RequestTag(TEXT("Item.Container"))},
			bOverwritePresetFields);
		{
			FYIItemContainerDefinitionFragment* Container = YI_FindOrAddFragment<FYIItemContainerDefinitionFragment>(ItemDefinition);
			if (Container)
			{
				Container->bIsContainerItem = true;
				if (bOverwritePresetFields || Container->ContainerDefaultGridSize.X <= 0 || Container->ContainerDefaultGridSize.Y <= 0)
				{
					Container->ContainerDefaultGridSize = FIntPoint(4, 4);
				}
			}
			FYIItemEquipmentDefinitionFragment* Equipment = YI_FindOrAddFragment<FYIItemEquipmentDefinitionFragment>(ItemDefinition);
			const FGameplayTag SpellbookSlot = YI_RequestTag(TEXT("Equipment.Spellbook"));
			if (Equipment && SpellbookSlot.IsValid())
			{
				if (bOverwritePresetFields || !Equipment->PrimaryEquipSlot.IsValid())
				{
					Equipment->PrimaryEquipSlot = SpellbookSlot;
				}
				if (!Equipment->OccupiedSlots.HasTagExact(SpellbookSlot))
				{
					Equipment->OccupiedSlots.AddTag(SpellbookSlot);
				}
			}
		}
		break;

	default:
		break;
	}

	YIItemSchema::InvalidateSnapshotCache(ItemDefinition);
	ItemDefinition->MarkPackageDirty();
#if WITH_EDITOR
	ItemDefinition->PostEditChange();
#endif
	return true;
}

