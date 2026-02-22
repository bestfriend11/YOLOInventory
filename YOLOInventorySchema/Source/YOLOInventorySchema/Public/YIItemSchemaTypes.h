#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "YIItemSchemaTypes.generated.h"

class UTexture2D;
class UScriptStruct;
struct FYIItemCustomDefinitionFragment;

/** Schema-facing display payload decoupled from legacy item definition fields. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaDisplayData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	TSoftObjectPtr<UTexture2D> Icon;
};

/** Classification payload (type, tags, rarity). */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaClassificationData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FGameplayTag ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FGameplayTag RarityTag;
};

/** Audio payload for item SFX routing. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaAudioData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FGameplayTag AudioTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FSoftObjectPath SoundProfileOverride;
};

/** Schema-facing layout payload used by grid and non-grid consumers. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaLayoutData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema", meta=(ClampMin="1"))
	FIntPoint DefaultSize = FIntPoint(1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	bool bAllowRotation = true;
};

/** Schema-facing stacking payload (independent from legacy field names). */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaStackingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	bool bAllowStacking = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema", meta=(ClampMin="1"))
	int32 MaxStackCount = 99;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	bool bUseRiskChecks = true;
};

/** Schema-facing affix generation payload that does not depend on legacy asset class headers. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaAffixData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	TArray<FSoftObjectPath> TemplateAffixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema", meta=(ClampMin="0"))
	int32 MinRandomModifiers = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema", meta=(ClampMin="0"))
	int32 MaxRandomModifiers = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FSoftObjectPath PrefixPool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FSoftObjectPath SuffixPool;
};

/** Equipment payload used by equipment-slot systems. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaEquipmentData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FGameplayTag PrimaryEquipSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FGameplayTagContainer OccupiedSlots;
};

/** Rule payload for uniqueness/capacity concerns. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaRulesData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	bool bUniquePerType = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema", meta=(ClampMin="1"))
	int32 EquipSlotCost = 1;
};

/** Container payload for nested bag scenarios. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaContainerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	bool bIsContainerItem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FSoftObjectPath ContainerTemplateBag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema", meta=(ClampMin="1"))
	FIntPoint ContainerDefaultGridSize = FIntPoint(6, 8);
};

/** Attribute-mod payload for item-driven stat grants. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaAttributeModsData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	TArray<FSoftObjectPath> AttributeMods;
};

/** Combined schema snapshot exported from legacy item definitions for suite-side consumers. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	int64 UniqueCode = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FString TemplateId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaDisplayData Display;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaClassificationData Classification;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaAudioData Audio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaLayoutData Layout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaStackingData Stacking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaAffixData Affix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaEquipmentData Equipment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaRulesData Rules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaContainerData Container;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaAttributeModsData AttributeMods;

	/**
	 * Resolved static fragments after inheritance/trait merge.
	 * This intentionally stores arbitrary fragment structs so future modules can extend
	 * schema authoring without changing core snapshot types.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema|Fragments")
	TArray<FInstancedStruct> ResolvedDefinitionFragments;

	/** Fast read index built when the snapshot is materialized. */
	TMap<const UScriptStruct*, int32> ResolvedFragmentIndexByStruct;
	TMultiMap<FGameplayTag, int32> ResolvedCustomFragmentIndexByTag;

	void RebuildResolvedFragmentIndex();
	const FInstancedStruct* FindResolvedFragmentByStruct(const UScriptStruct* FragmentStruct) const;
	const FYIItemCustomDefinitionFragment* FindResolvedCustomFragmentByTag(const FGameplayTag& FragmentTag) const;
	void FindResolvedCustomFragmentsByTag(const FGameplayTag& FragmentTag, TArray<const FYIItemCustomDefinitionFragment*>& OutFragments) const;

	template<typename TFragmentType>
	const TFragmentType* FindResolvedFragment() const
	{
		if (const FInstancedStruct* Fragment = FindResolvedFragmentByStruct(TFragmentType::StaticStruct()))
		{
			return Fragment->GetPtr<TFragmentType>();
		}
		return nullptr;
	}
};
