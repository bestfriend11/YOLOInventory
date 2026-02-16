#include "YIPhase2TestMapActor.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "InventoryScreenWidget.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIInventoryGameplaySetupLibrary.h"
#include "YIItemDefinition.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

AYIPhase2TestMapActor::AYIPhase2TestMapActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AYIPhase2TestMapActor::BeginPlay()
{
	Super::BeginPlay();

	if (bSetupOnBeginPlay)
	{
		RunSetupNow();
	}

	if (bOpenInventoryScreenOnBeginPlay)
	{
		HandleDeferredOpenInventory();
	}
}

void AYIPhase2TestMapActor::RunSetupNow()
{
	SetupAttemptCount = 0;
	const bool bSetupDone = TrySetupOnAuthority();
	if (bSetupDone)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(SetupRetryTimerHandle);
		}
		return;
	}
	RetrySetupIfNeeded();
}

void AYIPhase2TestMapActor::OpenInventoryForLocalPlayer()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DeferredOpenInventoryHandle);
	}

	if (IsRunningDedicatedServer())
	{
		return;
	}

	APawn* TargetPawn = ResolveTargetPawn();
	if (!TargetPawn)
	{
		EmitSetupMessage(TEXT("Phase2 map setup: no target pawn for opening inventory screen."), FColor::Orange);
		return;
	}

	UYIInventoryComponent* InventoryComp = TargetPawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InventoryComp)
	{
		EmitSetupMessage(FString::Printf(TEXT("Phase2 map setup: pawn '%s' has no UYIInventoryComponent."), *TargetPawn->GetName()), FColor::Orange);
		return;
	}

	if (!InventoryScreenClass.IsNull())
	{
		InventoryComp->InventoryScreenClass = InventoryScreenClass;
	}

	if (UInventoryScreenWidget* Screen = InventoryComp->OpenInventoryScreen())
	{
		EmitSetupMessage(FString::Printf(TEXT("Phase2 map setup: opened inventory screen '%s'."), *Screen->GetName()), FColor::Green);
	}
	else
	{
		EmitSetupMessage(TEXT("Phase2 map setup: failed to open inventory screen (set InventoryScreenClass on this actor or inventory component)."), FColor::Yellow);
	}
}

#if WITH_EDITOR
void AYIPhase2TestMapActor::CreatePresetBlueprintAssetFromCurrentSettings()
{
	if (PresetBlueprintFolder.IsEmpty() || PresetBlueprintName.IsEmpty())
	{
		EmitSetupMessage(TEXT("Preset creation failed: folder/name is empty."), FColor::Red);
		return;
	}

	if (!FPackageName::IsValidLongPackageName(PresetBlueprintFolder))
	{
		EmitSetupMessage(FString::Printf(TEXT("Preset creation failed: invalid folder '%s'."), *PresetBlueprintFolder), FColor::Red);
		return;
	}

	const FString BasePackageName = PresetBlueprintFolder / PresetBlueprintName;
	FString PackageName = BasePackageName;
	if (!FPackageName::IsValidObjectPath(PackageName + TEXT(".") + PresetBlueprintName))
	{
		EmitSetupMessage(FString::Printf(TEXT("Preset creation failed: invalid package/object '%s'."), *BasePackageName), FColor::Red);
		return;
	}

	if (FindObject<UObject>(nullptr, *(PackageName + TEXT(".") + PresetBlueprintName)))
	{
		const EAppReturnType::Type Replace = FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::FromString(FString::Printf(TEXT("Asset '%s' already exists. Create a unique name instead?"), *BasePackageName)));
		if (Replace != EAppReturnType::Yes)
		{
			return;
		}

		int32 Suffix = 1;
		while (FindObject<UObject>(nullptr, *((PresetBlueprintFolder / FString::Printf(TEXT("%s_%d"), *PresetBlueprintName, Suffix)) + TEXT(".") + FString::Printf(TEXT("%s_%d"), *PresetBlueprintName, Suffix))))
		{
			++Suffix;
		}
		PackageName = PresetBlueprintFolder / FString::Printf(TEXT("%s_%d"), *PresetBlueprintName, Suffix);
	}

	const FString AssetName = FPackageName::GetShortName(PackageName);
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		EmitSetupMessage(TEXT("Preset creation failed: package creation failed."), FColor::Red);
		return;
	}

	UBlueprint* BlueprintAsset = FKismetEditorUtilities::CreateBlueprint(
		StaticClass(),
		Package,
		*AssetName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		FName(TEXT("YOLOInventoryPhase2Preset")));
	if (!BlueprintAsset)
	{
		EmitSetupMessage(TEXT("Preset creation failed: blueprint creation failed."), FColor::Red);
		return;
	}

	FKismetEditorUtilities::CompileBlueprint(BlueprintAsset);
	if (AYIPhase2TestMapActor* BlueprintCDO = Cast<AYIPhase2TestMapActor>(BlueprintAsset->GeneratedClass ? BlueprintAsset->GeneratedClass->GetDefaultObject() : nullptr))
	{
		BlueprintCDO->bSetupOnBeginPlay = bSetupOnBeginPlay;
		BlueprintCDO->bOpenInventoryScreenOnBeginPlay = bOpenInventoryScreenOnBeginPlay;
		BlueprintCDO->OpenInventoryDelaySeconds = OpenInventoryDelaySeconds;
		BlueprintCDO->SetupRetryDelaySeconds = SetupRetryDelaySeconds;
		BlueprintCDO->MaxSetupRetries = MaxSetupRetries;
		BlueprintCDO->bResetBagsBeforeSetup = bResetBagsBeforeSetup;
		BlueprintCDO->MainBagName = MainBagName;
		BlueprintCDO->SpellbookBagName = SpellbookBagName;
		BlueprintCDO->DefaultMainBagGridSize = DefaultMainBagGridSize;
		BlueprintCDO->DefaultSpellbookGridSize = DefaultSpellbookGridSize;
		BlueprintCDO->MainBagRoleTag = MainBagRoleTag;
		BlueprintCDO->SpellbookBagRoleTag = SpellbookBagRoleTag;
		BlueprintCDO->MainBagTemplate = MainBagTemplate;
		BlueprintCDO->SpellbookBagTemplate = SpellbookBagTemplate;
		BlueprintCDO->StarterItems = StarterItems;
		BlueprintCDO->SpellbookAcceptedItemTypeTag = SpellbookAcceptedItemTypeTag;
		BlueprintCDO->SpellbookEquipSlotTag = SpellbookEquipSlotTag;
		BlueprintCDO->SpellbookActionSlotIndex = SpellbookActionSlotIndex;
		BlueprintCDO->InventoryScreenClass = InventoryScreenClass;
		BlueprintCDO->bShowScreenMessages = bShowScreenMessages;
		BlueprintCDO->TargetPawnOverride = nullptr;
	}

	BlueprintAsset->MarkPackageDirty();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(BlueprintAsset);

	EmitSetupMessage(FString::Printf(TEXT("Created preset blueprint: %s"), *PackageName), FColor::Green);
}
#endif

bool AYIPhase2TestMapActor::TrySetupOnAuthority()
{
	if (!HasAuthority())
	{
		return true;
	}

	APawn* TargetPawn = ResolveTargetPawn();
	if (!TargetPawn)
	{
		EmitSetupMessage(TEXT("Phase2 map setup: target pawn not found yet, retrying..."), FColor::Yellow);
		return false;
	}

	FYIInventoryGameplaySetupOptions SetupOptions;
	SetupOptions.bCreateMissingInventoryComponent = true;
	SetupOptions.bCreateMissingEquipmentComponent = true;
	SetupOptions.bCreateMissingActionBarComponent = true;
	SetupOptions.bEnsureAtLeastOneBag = false;
	SetupOptions.bEnableActionBarAutoBindFromEquipment = false;

	FYIInventoryGameplaySetupResult SetupResult;
	UYIInventoryGameplaySetupLibrary::EnsurePawnInventoryGameplaySetup(TargetPawn, SetupOptions, SetupResult);

	UYIInventoryComponent* InventoryComp = TargetPawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InventoryComp)
	{
		EmitSetupMessage(FString::Printf(TEXT("Phase2 map setup failed: pawn '%s' has no UYIInventoryComponent."), *TargetPawn->GetName()), FColor::Red);
		return true;
	}

	if (!InventoryScreenClass.IsNull())
	{
		InventoryComp->InventoryScreenClass = InventoryScreenClass;
	}

	if (bResetBagsBeforeSetup)
	{
		if (InventoryComp->EquippedBag)
		{
			InventoryComp->CloseBag(InventoryComp->EquippedBag);
		}
		InventoryComp->Bags.Reset();
		InventoryComp->EquippedBag = nullptr;
	}

	UYIInventoryBag* MainBag = nullptr;
	UYIInventoryBag* SpellbookBag = nullptr;

	if (const UYIInventoryBag* MainTemplate = MainBagTemplate.LoadSynchronous())
	{
		MainBag = CreateRuntimeBagFromTemplate(InventoryComp, MainTemplate, MainBagName, DefaultMainBagGridSize);
	}
	if (!MainBag)
	{
		MainBag = InventoryComp->CreateBag(MainBagName, DefaultMainBagGridSize);
	}

	if (const UYIInventoryBag* SpellbookTemplate = SpellbookBagTemplate.LoadSynchronous())
	{
		SpellbookBag = CreateRuntimeBagFromTemplate(InventoryComp, SpellbookTemplate, SpellbookBagName, DefaultSpellbookGridSize);
	}
	if (!SpellbookBag)
	{
		SpellbookBag = CreateRuntimeBagFromTemplate(InventoryComp, nullptr, SpellbookBagName, DefaultSpellbookGridSize);
	}

	if (!MainBag || !SpellbookBag)
	{
		EmitSetupMessage(TEXT("Phase2 map setup failed: could not create required bags."), FColor::Red);
		return true;
	}

	if (MainBagRoleTag.IsValid())
	{
		MainBag->BagRoleTag = MainBagRoleTag;
	}
	if (SpellbookBagRoleTag.IsValid())
	{
		SpellbookBag->BagRoleTag = SpellbookBagRoleTag;
	}

	SpellbookBag->bEnforceAcceptanceRules = true;
	SpellbookBag->RequiredItemTags.Reset();
	SpellbookBag->BlockedItemTags.Reset();
	SpellbookBag->AllowedDefinitionClasses.Reset();
	SpellbookBag->AllowedItemTypes.Reset();
	if (SpellbookAcceptedItemTypeTag.IsValid())
	{
		SpellbookBag->AllowedItemTypes.AddTag(SpellbookAcceptedItemTypeTag);
	}
	else
	{
		EmitSetupMessage(TEXT("Phase2 map setup warning: SpellbookAcceptedItemTypeTag is empty. Spellbook bag currently accepts any item type."), FColor::Yellow);
	}

	if (!InventoryComp->Bags.Contains(MainBag))
	{
		InventoryComp->Bags.Add(MainBag);
	}
	if (!InventoryComp->Bags.Contains(SpellbookBag))
	{
		InventoryComp->Bags.Add(SpellbookBag);
	}

	SeedStarterItems(MainBag, SpellbookBag);

	InventoryComp->OpenBag(MainBag);
	InventoryComp->SetActiveSpellbookBagById(SpellbookBag->BagId);
	InventoryComp->SyncNetState();

	if (SpellbookEquipSlotTag.IsValid())
	{
		FYIInventoryGameplaySetupResult PresetResult;
		UYIInventoryGameplaySetupLibrary::ApplySpellbookActionPreset(TargetPawn, SpellbookEquipSlotTag, SpellbookActionSlotIndex, PresetResult);
		if (!PresetResult.bSuccess)
		{
			for (const FString& Issue : PresetResult.BlockingIssues)
			{
				EmitSetupMessage(FString::Printf(TEXT("Phase2 map setup action preset error: %s"), *Issue), FColor::Red);
			}
		}
	}
	else
	{
		EmitSetupMessage(TEXT("Phase2 map setup warning: SpellbookEquipSlotTag is empty, action preset not applied."), FColor::Yellow);
	}

	FYIInventoryGameplaySetupResult ValidationResult;
	UYIInventoryGameplaySetupLibrary::ValidatePawnInventoryGameplaySetup(TargetPawn, ValidationResult);
	for (const FString& Issue : ValidationResult.BlockingIssues)
	{
		EmitSetupMessage(FString::Printf(TEXT("Phase2 validation error: %s"), *Issue), FColor::Red);
	}
	for (const FString& Warning : ValidationResult.Warnings)
	{
		EmitSetupMessage(FString::Printf(TEXT("Phase2 validation warning: %s"), *Warning), FColor::Yellow);
	}

	EmitSetupMessage(FString::Printf(
		TEXT("Phase2 map setup completed for pawn '%s'. MainBag='%s' Spellbook='%s'"),
		*TargetPawn->GetName(),
		*MainBag->GetName(),
		*SpellbookBag->GetName()),
		FColor::Green);
	return true;
}

APawn* AYIPhase2TestMapActor::ResolveTargetPawn() const
{
	if (TargetPawnOverride)
	{
		return TargetPawnOverride;
	}

	if (!GetWorld())
	{
		return nullptr;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (APawn* PCPawn = PC->GetPawn())
		{
			return PCPawn;
		}
	}

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		if (APawn* Pawn = *It)
		{
			return Pawn;
		}
	}

	return nullptr;
}

UYIInventoryBag* AYIPhase2TestMapActor::CreateRuntimeBagFromTemplate(
	UYIInventoryComponent* InventoryComp,
	const UYIInventoryBag* TemplateBag,
	const FName FallbackName,
	const FIntPoint FallbackGridSize) const
{
	if (!InventoryComp)
	{
		return nullptr;
	}

	UYIInventoryBag* NewBag = NewObject<UYIInventoryBag>(InventoryComp, NAME_None, RF_Transactional);
	if (!NewBag)
	{
		return nullptr;
	}

	if (TemplateBag)
	{
		NewBag->DisplayName = TemplateBag->DisplayName;
		NewBag->GridSize = TemplateBag->GridSize;
		NewBag->CellPixelSize = TemplateBag->CellPixelSize;
		NewBag->bAllowRotation = TemplateBag->bAllowRotation;
		NewBag->MinifyScale = TemplateBag->MinifyScale;
		NewBag->GridStyleAsset = TemplateBag->GridStyleAsset;
		NewBag->GridLineColor = TemplateBag->GridLineColor;
		NewBag->OuterLineColor = TemplateBag->OuterLineColor;
		NewBag->CellBgColor = TemplateBag->CellBgColor;
		NewBag->GridThickness = TemplateBag->GridThickness;
		NewBag->bShowCellTooltips = TemplateBag->bShowCellTooltips;
		NewBag->bShowSortingHeaders = TemplateBag->bShowSortingHeaders;
		NewBag->bEnableThumbnails = TemplateBag->bEnableThumbnails;
		NewBag->bEnableHoverHighlight = TemplateBag->bEnableHoverHighlight;
		NewBag->bUseTagFilter = TemplateBag->bUseTagFilter;
		NewBag->TagFilters = TemplateBag->TagFilters;
		NewBag->bUseFolderFilter = TemplateBag->bUseFolderFilter;
		NewBag->FolderFilters = TemplateBag->FolderFilters;
		NewBag->Items = TemplateBag->Items;
		NewBag->bAutoMergeOnAdd = TemplateBag->bAutoMergeOnAdd;
		NewBag->bEnforceAcceptanceRules = TemplateBag->bEnforceAcceptanceRules;
		NewBag->AllowedItemTypes = TemplateBag->AllowedItemTypes;
		NewBag->RequiredItemTags = TemplateBag->RequiredItemTags;
		NewBag->BlockedItemTags = TemplateBag->BlockedItemTags;
		NewBag->AllowedDefinitionClasses = TemplateBag->AllowedDefinitionClasses;
	}
	else
	{
		NewBag->DisplayName = FText::FromName(FallbackName);
		NewBag->GridSize = FallbackGridSize;
	}

	if (NewBag->DisplayName.IsEmpty())
	{
		NewBag->DisplayName = FText::FromName(FallbackName);
	}
	if (NewBag->GridSize.X <= 0 || NewBag->GridSize.Y <= 0)
	{
		NewBag->GridSize = FallbackGridSize;
	}

	if (!InventoryComp->Bags.Contains(NewBag))
	{
		InventoryComp->Bags.Add(NewBag);
	}
	NewBag->EnsureBagId();
	return NewBag;
}

void AYIPhase2TestMapActor::SeedStarterItems(UYIInventoryBag* MainBag, UYIInventoryBag* SpellbookBag) const
{
	if (!MainBag || !SpellbookBag)
	{
		return;
	}

	for (const FYIPhase2StarterItemEntry& Entry : StarterItems)
	{
		UYIItemDefinition* Def = Entry.ItemDefinition.IsValid() ? Entry.ItemDefinition.Get() : Entry.ItemDefinition.LoadSynchronous();
		if (!Def)
		{
			continue;
		}

		UYIInventoryBag* TargetBag = (Entry.TargetBag == EYIPhase2StarterTargetBag::Spellbook) ? SpellbookBag : MainBag;
		FYIBagItem NewItem;
		NewItem.Item.Definition = Def;
		NewItem.Item.Count = FMath::Max(1, Entry.Count);
		NewItem.Size = Def->DefaultSize;
		NewItem.Pos = FIntPoint::ZeroValue;

		const int32 NewIndex = TargetBag->AddBagItem(NewItem);
		if (NewIndex == INDEX_NONE)
		{
			EmitSetupMessage(FString::Printf(TEXT("Phase2 setup warning: failed to add starter item '%s' into bag '%s'."),
				*Def->GetName(), *TargetBag->GetName()), FColor::Yellow);
		}
	}
}

void AYIPhase2TestMapActor::EmitSetupMessage(const FString& Message, const FColor& Color) const
{
	UE_LOG(LogTemp, Display, TEXT("[YI Phase2 TestMap] %s"), *Message);
	if (!bShowScreenMessages || !GEngine)
	{
		return;
	}

	const uint64 Key = GetTypeHash(Message);
	GEngine->AddOnScreenDebugMessage((int32)(Key & 0x7fffffff), 10.f, Color, Message);
}

void AYIPhase2TestMapActor::RetrySetupIfNeeded()
{
	if (!HasAuthority() || !GetWorld())
	{
		return;
	}

	++SetupAttemptCount;
	if (SetupAttemptCount > MaxSetupRetries)
	{
		EmitSetupMessage(TEXT("Phase2 map setup aborted: max retries reached before pawn became available."), FColor::Red);
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		SetupRetryTimerHandle,
		this,
		&AYIPhase2TestMapActor::HandleRetrySetupTimer,
		SetupRetryDelaySeconds,
		false);
}

void AYIPhase2TestMapActor::HandleRetrySetupTimer()
{
	if (!TrySetupOnAuthority())
	{
		RetrySetupIfNeeded();
	}
}

void AYIPhase2TestMapActor::HandleDeferredOpenInventory()
{
	if (!GetWorld() || IsRunningDedicatedServer())
	{
		return;
	}

	if (OpenInventoryDelaySeconds <= 0.f)
	{
		OpenInventoryForLocalPlayer();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		DeferredOpenInventoryHandle,
		this,
		&AYIPhase2TestMapActor::OpenInventoryForLocalPlayer,
		OpenInventoryDelaySeconds,
		false);
}
