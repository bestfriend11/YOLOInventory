#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIAffix.h"
#include "StructUtils/InstancedStruct.h"
#include "YIItemFragments.h"
#include "YIItemInstance.generated.h"

class UYIItemDefinition;

// Runtime representation of an item stack/instance
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemInstance
{
	GENERATED_BODY()

	// Definition of this item (authoring)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TSoftObjectPtr<UYIItemDefinition> Definition;

	// Stack count
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	int32 Count = 1;

	// Unique ID for this specific instance (for persistence)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item", meta=(IgnoreForMemberInitializationTest))
	FGuid InstanceId;

	// Unique ID that identifies this stack in a container (stacks of same def share a StackId)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item", meta=(IgnoreForMemberInitializationTest))
	FGuid StackId;

	// Optional custom stack key (hash) to control merge rules beyond Definition
	// Optional 64-bit numeric custom key for stacking compatibility
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	int64 CustomStackKey = 0;

	// Optional nested-container linkage. When valid, this item points to a runtime bag instance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Container")
	FGuid ContainedBagId;

	// Rotated layout flag for grid containers (size is derived from the Definition->DefaultSize)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout")
	bool bRotated = false; 

	// Per-instance dynamic attributes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attributes")
	TMap<FName,float> Attributes;

	// Affixes rolled on this instance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes")
	TArray<FYIAffixInstance> Affixes;

	/**
	 * Modular runtime payloads (fragment-style), using FInstancedStruct to avoid UObject-per-item overhead.
	 * Authoring stays in shared data assets; only per-instance mutable payload lives here.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragments", meta=(BaseStruct="/Script/YOLOInventory.YIItemFragmentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> Fragments;

	// Runtime capability states (if needed by systems)
	UPROPERTY(EditAnywhere, Category="Capabilities")
	// Deprecated legacy runtime cap payload removed; blueprint capabilities should manage their own runtime state
	TArray<uint8> DeprecatedRuntimeCaps_DoNotUse;

	FYIItemInstance()
	{
		InstanceId = FGuid::NewGuid();
		StackId = FGuid::NewGuid();
	}

	/** Copies legacy Affixes/Attributes into fragment payloads (non-destructive). */
	void SyncLegacyToCoreFragments();

	/** Copies fragment payloads back into legacy Affixes/Attributes for compatibility with old code paths. */
	void SyncCoreFragmentsToLegacy();

	const FYIItemAffixesFragment* GetAffixesFragment() const;
	FYIItemAffixesFragment* GetMutableAffixesFragment(bool bCreateIfMissing);

	const FYIItemAttributesFragment* GetAttributesFragment() const;
	FYIItemAttributesFragment* GetMutableAttributesFragment(bool bCreateIfMissing);

	const FYIItemDurabilityFragment* GetDurabilityFragment() const;
	FYIItemDurabilityFragment* GetMutableDurabilityFragment(bool bCreateIfMissing);
};
