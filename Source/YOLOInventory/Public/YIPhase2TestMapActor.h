#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "YIPhase2TestMapActor.generated.h"

class APawn;
class UInventoryScreenWidget;
class UYIInventoryBag;
class UYIInventoryComponent;
class UYIItemDefinition;

UENUM(BlueprintType)
enum class EYIPhase2StarterTargetBag : uint8
{
	MainInventory UMETA(DisplayName="Main Inventory"),
	Spellbook UMETA(DisplayName="Spellbook")
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIPhase2StarterItemEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2")
	TSoftObjectPtr<UYIItemDefinition> ItemDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2", meta=(ClampMin="1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2")
	EYIPhase2StarterTargetBag TargetBag = EYIPhase2StarterTargetBag::MainInventory;
};

/**
 * Runtime bootstrap actor used to quickly validate Phase 2 inventory + spellbook workflows in a playable map.
 * Place this actor in a test level, press Play, and it configures the active pawn automatically.
 */
UCLASS(Blueprintable)
class YOLOINVENTORY_API AYIPhase2TestMapActor : public AActor
{
	GENERATED_BODY()
public:
	AYIPhase2TestMapActor();

	virtual void BeginPlay() override;

	/** Run setup immediately (safe to call in PIE; authority required for bag/equipment mutation). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="YOLOInventory|Phase2")
	void RunSetupNow();

	/** Opens inventory screen for local player if setup already exists. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Phase2")
	void OpenInventoryForLocalPlayer();

#if WITH_EDITOR
	/** Creates a reusable Blueprint preset asset derived from this actor, copying current values from this instance. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="YOLOInventory|Phase2")
	void CreatePresetBlueprintAssetFromCurrentSettings();
#endif

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Flow")
	bool bSetupOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Flow")
	bool bOpenInventoryScreenOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Flow", meta=(ClampMin="0.0"))
	float OpenInventoryDelaySeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Flow", meta=(ClampMin="0.0"))
	float SetupRetryDelaySeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Flow", meta=(ClampMin="0", ClampMax="16"))
	int32 MaxSetupRetries = 8;

	/** Optional explicit pawn target. If unset, Player0 pawn is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Flow")
	TObjectPtr<APawn> TargetPawnOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Inventory")
	bool bResetBagsBeforeSetup = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Inventory")
	FName MainBagName = TEXT("Inventory");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Inventory")
	FName SpellbookBagName = TEXT("Spellbook");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Inventory")
	FIntPoint DefaultMainBagGridSize = FIntPoint(10, 6);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Inventory")
	FIntPoint DefaultSpellbookGridSize = FIntPoint(4, 4);

	/** Optional runtime role tag written to the created main bag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Inventory")
	FGameplayTag MainBagRoleTag;

	/** Optional runtime role tag written to the created spellbook bag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Inventory")
	FGameplayTag SpellbookBagRoleTag;

	/** Optional bag templates copied into runtime owned bags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Inventory")
	TSoftObjectPtr<UYIInventoryBag> MainBagTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Inventory")
	TSoftObjectPtr<UYIInventoryBag> SpellbookBagTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Inventory")
	TArray<FYIPhase2StarterItemEntry> StarterItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Spellbook")
	FGameplayTag SpellbookAcceptedItemTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Spellbook")
	FGameplayTag SpellbookEquipSlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Spellbook", meta=(ClampMin="0"))
	int32 SpellbookActionSlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|UI")
	TSoftClassPtr<UInventoryScreenWidget> InventoryScreenClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase2|Debug")
	bool bShowScreenMessages = true;

#if WITH_EDITORONLY_DATA
	/** Destination folder for preset blueprint creation. Must be a valid game content path (for example /Game/YOLOInventory/Presets). */
	UPROPERTY(EditAnywhere, Category="Phase2|Preset")
	FString PresetBlueprintFolder = TEXT("/Game/YOLOInventory/Presets");

	/** Asset name to create in PresetBlueprintFolder. */
	UPROPERTY(EditAnywhere, Category="Phase2|Preset")
	FString PresetBlueprintName = TEXT("BP_YIPhase2TestMapActor");
#endif

private:
	bool TrySetupOnAuthority();
	APawn* ResolveTargetPawn() const;

	UYIInventoryBag* CreateRuntimeBagFromTemplate(
		UYIInventoryComponent* InventoryComp,
		const UYIInventoryBag* TemplateBag,
		const FName FallbackName,
		const FIntPoint FallbackGridSize) const;

	void SeedStarterItems(UYIInventoryBag* MainBag, UYIInventoryBag* SpellbookBag) const;
	void EmitSetupMessage(const FString& Message, const FColor& Color) const;

	void RetrySetupIfNeeded();
	void HandleRetrySetupTimer();
	void HandleDeferredOpenInventory();

	mutable int32 SetupAttemptCount = 0;
	FTimerHandle SetupRetryTimerHandle;
	FTimerHandle DeferredOpenInventoryHandle;
};
