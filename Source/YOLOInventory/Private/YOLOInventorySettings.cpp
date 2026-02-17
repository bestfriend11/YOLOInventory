#include "YOLOInventorySettings.h"
#include "YIInventoryTypes.h"

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
		AddDefault(TEXT("Rarity.Common"), YI_GetRarityColor(EYOLOItemRarity::Common));
		AddDefault(TEXT("Rarity.Uncommon"), YI_GetRarityColor(EYOLOItemRarity::Uncommon));
		AddDefault(TEXT("Rarity.Rare"), YI_GetRarityColor(EYOLOItemRarity::Rare));
		AddDefault(TEXT("Rarity.Epic"), YI_GetRarityColor(EYOLOItemRarity::Epic));
		AddDefault(TEXT("Rarity.Legendary"), YI_GetRarityColor(EYOLOItemRarity::Legendary));
		AddDefault(TEXT("Rarity.Mythic"), YI_GetRarityColor(EYOLOItemRarity::Mythic));
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
