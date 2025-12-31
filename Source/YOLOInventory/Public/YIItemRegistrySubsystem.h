#pragma once
#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "YIItemRegistrySubsystem.generated.h"

class UYIItemDefinition;

UCLASS()
class YOLOINVENTORY_API UYIItemRegistrySubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Build or rebuild the index by scanning asset registry for UYIItemDefinition
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Registry")
	void BuildIndex(bool bForce = false);

	// Find an item definition by its UniqueCode. Returns nullptr if not found.
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Registry")
	UYIItemDefinition* GetByCode(int64 Code);

	// Ensures codes are unique. If bAutoFix, assigns new random codes for duplicates or zero codes (editor-only recommended)
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Registry")
	bool EnsureUniqueCodes(bool bAutoFix);

private:
	TMap<int64, TSoftObjectPtr<UYIItemDefinition>> CodeToAsset;
	bool bIndexed = false;
};
