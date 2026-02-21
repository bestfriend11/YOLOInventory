#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "YIItemSchemaWarmupSubsystem.generated.h"

struct FAssetData;
struct FSoftObjectPath;
struct FStreamableHandle;

/**
 * Automatically preloads item schema assets and prebuilds resolver snapshots at engine startup.
 * No designer wiring is required.
 */
UCLASS()
class YOLOINVENTORYSCHEMA_API UYIItemSchemaWarmupSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category="YOLOInventory|Schema")
	bool IsWarmupComplete() const { return bWarmupComplete; }

private:
	void StartWarmup();
	void OnWarmupAssetsLoaded();
	void BuildWarmSnapshots();
	void GatherWarmupAssetPaths(TArray<FSoftObjectPath>& OutPaths) const;

	TArray<FSoftObjectPath> WarmupPaths;
	TSharedPtr<FStreamableHandle> WarmupHandle;
	bool bWarmupComplete = false;
};
