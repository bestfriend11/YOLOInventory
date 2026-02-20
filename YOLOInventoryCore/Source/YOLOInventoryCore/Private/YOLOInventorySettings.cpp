#include "YOLOInventorySettings.h"

UYOLOInventorySettings::UYOLOInventorySettings()
{
	if (RarityColors.Num() == 0)
	{
		auto AddDefault = [this](const TCHAR* TagName, const FLinearColor& Color)
		{
			FYIRarityColorEntry Entry;
			Entry.RarityTag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
			Entry.Color = Color;
			RarityColors.Add(Entry);
		};
		AddDefault(TEXT("Rarity.Common"), FLinearColor(0.70f, 0.70f, 0.70f, 1.f));
		AddDefault(TEXT("Rarity.Uncommon"), FLinearColor(0.20f, 0.95f, 0.20f, 1.f));
		AddDefault(TEXT("Rarity.Rare"), FLinearColor(0.20f, 0.45f, 1.00f, 1.f));
		AddDefault(TEXT("Rarity.Epic"), FLinearColor(0.70f, 0.35f, 1.00f, 1.f));
		AddDefault(TEXT("Rarity.Legendary"), FLinearColor(1.00f, 0.58f, 0.15f, 1.f));
		AddDefault(TEXT("Rarity.Mythic"), FLinearColor(1.00f, 0.25f, 0.25f, 1.f));
		AddDefault(TEXT("Rarity.Unique"), FLinearColor(0.95f, 0.55f, 0.15f, 1.f));
	}

	if (SuggestedInventoryTagPrefixes.Num() == 0)
	{
		SuggestedInventoryTagPrefixes = {
			TEXT("Inventory."),
			TEXT("Item."),
			TEXT("Equip."),
			TEXT("Bag."),
			TEXT("Actions."),
			TEXT("Affix."),
			TEXT("Loot."),
			TEXT("Craft."),
			TEXT("Rarity."),
			TEXT("Audio.")
		};
	}
}

const UYOLOInventorySettings& UYOLOInventorySettings::Get()
{
	return *GetDefault<UYOLOInventorySettings>();
}

UYOLOInventorySettings& UYOLOInventorySettings::GetMutable()
{
	return *GetMutableDefault<UYOLOInventorySettings>();
}

bool UYOLOInventorySettings::IsDebugChannelEnabled(EYIDebugChannel Channel) const
{
	switch (Channel)
	{
	case EYIDebugChannel::General: return bDebugChannelGeneral;
	case EYIDebugChannel::Persistence: return bDebugChannelPersistence;
	case EYIDebugChannel::Inventory: return bDebugChannelInventory;
	case EYIDebugChannel::Equipment: return bDebugChannelEquipment;
	case EYIDebugChannel::ActionBar: return bDebugChannelActionBar;
	case EYIDebugChannel::Trade: return bDebugChannelTrade;
	case EYIDebugChannel::Shop: return bDebugChannelShop;
	case EYIDebugChannel::Grid: return bDebugChannelGrid;
	case EYIDebugChannel::Phase2: return bDebugChannelPhase2;
	default: return true;
	}
}

#if WITH_EDITOR
FName UYOLOInventorySettings::GetCategoryName() const
{
	return TEXT("Plugins");
}
#endif
