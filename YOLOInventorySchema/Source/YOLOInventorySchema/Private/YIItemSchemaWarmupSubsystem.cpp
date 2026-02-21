#include "YIItemSchemaWarmupSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "YIItemTraitAsset.h"

void UYIItemSchemaWarmupSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartWarmup();
}

void UYIItemSchemaWarmupSubsystem::Deinitialize()
{
	WarmupHandle.Reset();
	WarmupPaths.Reset();
	bWarmupComplete = false;
	Super::Deinitialize();
}

void UYIItemSchemaWarmupSubsystem::StartWarmup()
{
	bWarmupComplete = false;
	WarmupPaths.Reset();
	GatherWarmupAssetPaths(WarmupPaths);

	if (WarmupPaths.Num() == 0)
	{
		bWarmupComplete = true;
		return;
	}

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	WarmupHandle = Streamable.RequestAsyncLoad(
		WarmupPaths,
		FStreamableDelegate::CreateUObject(this, &UYIItemSchemaWarmupSubsystem::OnWarmupAssetsLoaded),
		FStreamableManager::AsyncLoadHighPriority);

	if (!WarmupHandle.IsValid())
	{
		// Fallback for unusual startup order where async load could not be scheduled.
		BuildWarmSnapshots();
		bWarmupComplete = true;
	}
}

void UYIItemSchemaWarmupSubsystem::OnWarmupAssetsLoaded()
{
	BuildWarmSnapshots();
	bWarmupComplete = true;
	WarmupHandle.Reset();
}

void UYIItemSchemaWarmupSubsystem::BuildWarmSnapshots()
{
	TArray<UObject*> LoadedAssets;
	if (WarmupHandle.IsValid())
	{
		WarmupHandle->GetLoadedAssets(LoadedAssets);
	}
	else
	{
		LoadedAssets.Reserve(WarmupPaths.Num());
		for (const FSoftObjectPath& Path : WarmupPaths)
		{
			if (UObject* Loaded = Path.TryLoad())
			{
				LoadedAssets.Add(Loaded);
			}
		}
	}

	for (UObject* Asset : LoadedAssets)
	{
		if (const UYIItemDefinition* Definition = Cast<UYIItemDefinition>(Asset))
		{
			YIItemSchema::WarmupDefinition(Definition);
		}
	}
}

void UYIItemSchemaWarmupSubsystem::GatherWarmupAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	OutPaths.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	auto GatherByClass = [&AssetRegistry, &OutPaths](const FTopLevelAssetPath& ClassPath)
	{
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByClass(ClassPath, Assets, true);
		for (const FAssetData& AssetData : Assets)
		{
			const FSoftObjectPath ObjectPath = AssetData.ToSoftObjectPath();
			if (ObjectPath.IsValid())
			{
				OutPaths.AddUnique(ObjectPath);
			}
		}
	};

	GatherByClass(UYIItemDefinition::StaticClass()->GetClassPathName());
	GatherByClass(UYIItemTraitAsset::StaticClass()->GetClassPathName());
}
