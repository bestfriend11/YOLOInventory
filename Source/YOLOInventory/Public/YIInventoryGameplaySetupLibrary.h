#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YIActionBarComponent.h"
#include "YIInventoryGameplaySetupLibrary.generated.h"

class APawn;
class UYIEquipmentLayoutAsset;
class UYIEquipmentComponent;
class UYIItemDefinition;

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIInventoryGameplaySetupOptions
{
	GENERATED_BODY()

	/** Create missing UYIInventoryComponent at runtime when needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup")
	bool bCreateMissingInventoryComponent = true;

	/** Create missing UYIEquipmentComponent at runtime when needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup")
	bool bCreateMissingEquipmentComponent = true;

	/** Create missing UYIActionBarComponent at runtime when needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup")
	bool bCreateMissingActionBarComponent = true;

	/** Ensure pawn has an open bag after setup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup")
	bool bEnsureAtLeastOneBag = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup", meta=(EditCondition="bEnsureAtLeastOneBag", EditConditionHides))
	FString DefaultBagName = TEXT("Inventory");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup", meta=(EditCondition="bEnsureAtLeastOneBag", EditConditionHides))
	FIntPoint DefaultBagGridSize = FIntPoint(10, 6);

	/** Ensure action bar has this many slots (minimum 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup", meta=(ClampMin="1"))
	int32 NumActionSlots = 10;

	/** Enable equipment-driven auto-binding in action bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup")
	bool bEnableActionBarAutoBindFromEquipment = true;

	/** Auto-bind rules copied into UYIActionBarComponent::AutoBindRules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup", meta=(EditCondition="bEnableActionBarAutoBindFromEquipment", EditConditionHides))
	TArray<FYIEquipmentActionAutoBindRule> AutoBindRules;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIInventoryGameplaySetupResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Result")
	bool bSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Result")
	TArray<FString> BlockingIssues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Result")
	TArray<FString> Warnings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Result")
	FString Summary;
};

UCLASS()
class YOLOINVENTORY_API UYIInventoryGameplaySetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * One-shot bootstrap for pawn inventory gameplay wiring.
	 * Intended for server-side use (BeginPlay/Possessed).
	 */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Setup", meta=(BlueprintAuthorityOnly="true"))
	static bool EnsurePawnInventoryGameplaySetup(APawn* Pawn, const FYIInventoryGameplaySetupOptions& Options, FYIInventoryGameplaySetupResult& OutResult);

	/** Runtime quick-start: ensures setup on authority and opens screen on local clients when possible. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Setup")
	static bool QuickStartPawnInventory(APawn* Pawn, bool bOpenInventoryScreen, FYIInventoryGameplaySetupResult& OutResult);

	/** Run setup diagnostics without mutating components. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Setup")
	static bool ValidatePawnInventoryGameplaySetup(APawn* Pawn, FYIInventoryGameplaySetupResult& OutResult);

	/** Convenience preset for spellbook slot -> action bar slot mapping. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Setup", meta=(BlueprintAuthorityOnly="true"))
	static bool ApplySpellbookActionPreset(APawn* Pawn, FGameplayTag SpellbookEquipSlotTag, int32 ActionSlotIndex, FYIInventoryGameplaySetupResult& OutResult);

	/** Build/update an equipment layout asset directly from equipment slot definitions (single source of truth). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Setup|Equipment")
	static bool SyncEquipmentLayoutFromComponentSlots(UYIEquipmentComponent* EquipmentComp, UYIEquipmentLayoutAsset* LayoutAsset, bool bClearExisting = true);

	/** Make an item explicitly support a slot tag so equip checks pass consistently. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Setup|Equipment")
	static bool EnsureItemSupportsEquipSlot(UYIItemDefinition* ItemDef, FGameplayTag SlotTag, bool bSetItemTypeToSlotTag = false, bool bAddToOccupiedSlots = false);
};
