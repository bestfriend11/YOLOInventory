#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "YIInventoryTypes.generated.h"

// Base enums and structs must be declared before they are referenced by other USTRUCTs

UENUM(BlueprintType)
enum class EYOLOItemRarity : uint8
{
    Common      UMETA(DisplayName = "Common"),
    Uncommon    UMETA(DisplayName = "Uncommon"),
    Rare        UMETA(DisplayName = "Rare"),
    Epic        UMETA(DisplayName = "Epic"),
    Legendary   UMETA(DisplayName = "Legendary"),
    Mythic      UMETA(DisplayName = "Mythic")
};

UENUM(BlueprintType)
enum class EYOLOItemType : uint8
{
    Weapon      UMETA(DisplayName="Weapon"),
    Armor       UMETA(DisplayName="Armor"),
    Trinket     UMETA(DisplayName="Trinket"),
    Consumable  UMETA(DisplayName="Consumable"),
    Material    UMETA(DisplayName="Material"),
    Quest       UMETA(DisplayName="Quest"),
    Misc        UMETA(DisplayName="Misc")
};

UENUM(BlueprintType)
enum class EYOLOEquipmentSlot : uint8
{
    None        UMETA(DisplayName="None"),
    WeaponMain  UMETA(DisplayName="Weapon Main"),
    WeaponOff   UMETA(DisplayName="Weapon Off"),
    Head        UMETA(DisplayName="Head"),
    Chest       UMETA(DisplayName="Chest"),
    Hands       UMETA(DisplayName="Hands"),
    Legs        UMETA(DisplayName="Legs"),
    Feet        UMETA(DisplayName="Feet"),
    Ring        UMETA(DisplayName="Ring"),
    Amulet      UMETA(DisplayName="Amulet"),
    Belt        UMETA(DisplayName="Belt"),
    Trinket     UMETA(DisplayName="Trinket")
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemStat
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Stats") FName StatName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Stats") float Value = 0.f;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemRequirement
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Requirements", meta=(FullyExpandStruct)) FGameplayAttribute Attribute;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Requirements") float MinValue = 0.f;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemAbility
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Abilities") TSoftClassPtr<UObject> AbilityClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Abilities") int32 Level = 1;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemEffect
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Effects") TSoftClassPtr<UObject> EffectClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Effects") float Magnitude = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Effects") float Duration = 0.f;
};

// Lightweight per-stack capability state example
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIEvolutionState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capability|Evolution") int32 XP = 0;
};

// Variant unlock removed for now

// Full variant snapshot
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemVariant
{
    GENERATED_BODY()

    // Identity
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Identity") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Identity") FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Identity") EYOLOItemRarity Rarity = EYOLOItemRarity::Common;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Identity") TSoftObjectPtr<UTexture2D> Icon;

    // Classification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Class") EYOLOItemType ItemType = EYOLOItemType::Misc;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Class") EYOLOEquipmentSlot Slot = EYOLOEquipmentSlot::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Class") int32 ItemLevel = 1;

    // Layout/Economy
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Economy") FIntPoint DefaultSize = FIntPoint(1,1);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Economy") int32 Price = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Economy") int32 SellPrice = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Economy") float Weight = 0.f;

    // Stacking rules
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Stacking") bool bAllowStacking = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Stacking", meta=(ClampMin="1")) int32 MaxStackCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Stacking") bool bUniquePerType = false;

    // Gameplay
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Gameplay") TArray<FYIItemRequirement> Requirements;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Gameplay") TArray<FYIItemStat> Stats;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Gameplay") TArray<FYIItemAbility> Abilities;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Gameplay") TArray<FYIItemEffect> Effects;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Gameplay") FGameplayTagContainer Tags;

    // Durability
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Durability", meta=(ClampMin="0")) int32 MaxDurability = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Variant|Durability", meta=(ClampMin="0")) int32 StartDurability = 0;

    // Unlock removed
};

// Rarity style
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYOLORarityStyle
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity")
    FLinearColor Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity")
    FText DisplayName;

    FYOLORarityStyle() : Color(FLinearColor::White), DisplayName(FText::GetEmpty()) {}
};

inline FLinearColor YI_GetRarityColor(EYOLOItemRarity Rarity)
{
    switch (Rarity)
    {
    default:
    case EYOLOItemRarity::Common:    return FLinearColor(0.85f,0.85f,0.85f,1.f);
    case EYOLOItemRarity::Uncommon:  return FLinearColor(0.25f,0.85f,0.35f,1.f);
    case EYOLOItemRarity::Rare:      return FLinearColor(0.2f,0.45f,1.0f,1.f);
    case EYOLOItemRarity::Epic:      return FLinearColor(0.6f,0.2f,0.9f,1.f);
    case EYOLOItemRarity::Legendary: return FLinearColor(1.0f,0.6f,0.1f,1.f);
    case EYOLOItemRarity::Mythic:    return FLinearColor(1.0f,0.2f,0.2f,1.f);
    }
}
