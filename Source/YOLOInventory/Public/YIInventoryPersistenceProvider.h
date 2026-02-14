#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/Function.h"
#include "YIInventoryPersistenceProvider.generated.h"

class UObject;
class UYIInventorySaveGame;

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORY_API UYIInventoryPersistenceProviderBase : public UObject
{
	GENERATED_BODY()
public:
	virtual bool DoesSaveExist(UObject* WorldContextObject, const FString& SlotName, int32 UserIndex) const;
	virtual UYIInventorySaveGame* Load(UObject* WorldContextObject, const FString& SlotName, int32 UserIndex);
	virtual void SaveAsync(
		UObject* WorldContextObject,
		UYIInventorySaveGame* SaveObject,
		const FString& SlotName,
		int32 UserIndex,
		TFunction<void(bool)> CompletionCallback);
};

/** Default disk backend using Unreal SaveGame slots. */
UCLASS(BlueprintType, EditInlineNew)
class YOLOINVENTORY_API UYISaveGameInventoryPersistenceProvider : public UYIInventoryPersistenceProviderBase
{
	GENERATED_BODY()
public:
	virtual bool DoesSaveExist(UObject* WorldContextObject, const FString& SlotName, int32 UserIndex) const override;
	virtual UYIInventorySaveGame* Load(UObject* WorldContextObject, const FString& SlotName, int32 UserIndex) override;
	virtual void SaveAsync(
		UObject* WorldContextObject,
		UYIInventorySaveGame* SaveObject,
		const FString& SlotName,
		int32 UserIndex,
		TFunction<void(bool)> CompletionCallback) override;
};
