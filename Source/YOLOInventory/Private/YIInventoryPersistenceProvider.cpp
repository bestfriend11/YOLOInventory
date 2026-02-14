#include "YIInventoryPersistenceProvider.h"

#include "Kismet/GameplayStatics.h"
#include "YIInventorySaveGame.h"

bool UYIInventoryPersistenceProviderBase::DoesSaveExist(UObject* WorldContextObject, const FString& SlotName, int32 UserIndex) const
{
	(void)WorldContextObject;
	(void)SlotName;
	(void)UserIndex;
	return false;
}

UYIInventorySaveGame* UYIInventoryPersistenceProviderBase::Load(UObject* WorldContextObject, const FString& SlotName, int32 UserIndex)
{
	(void)WorldContextObject;
	(void)SlotName;
	(void)UserIndex;
	return nullptr;
}

void UYIInventoryPersistenceProviderBase::SaveAsync(
	UObject* WorldContextObject,
	UYIInventorySaveGame* SaveObject,
	const FString& SlotName,
	int32 UserIndex,
	TFunction<void(bool)> CompletionCallback)
{
	(void)WorldContextObject;
	(void)SaveObject;
	(void)SlotName;
	(void)UserIndex;
	if (CompletionCallback)
	{
		CompletionCallback(false);
	}
}

bool UYISaveGameInventoryPersistenceProvider::DoesSaveExist(UObject* WorldContextObject, const FString& SlotName, int32 UserIndex) const
{
	(void)WorldContextObject;
	return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

UYIInventorySaveGame* UYISaveGameInventoryPersistenceProvider::Load(UObject* WorldContextObject, const FString& SlotName, int32 UserIndex)
{
	(void)WorldContextObject;
	if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex))
	{
		return Cast<UYIInventorySaveGame>(Loaded);
	}
	return nullptr;
}

void UYISaveGameInventoryPersistenceProvider::SaveAsync(
	UObject* WorldContextObject,
	UYIInventorySaveGame* SaveObject,
	const FString& SlotName,
	int32 UserIndex,
	TFunction<void(bool)> CompletionCallback)
{
	(void)WorldContextObject;
	if (!SaveObject)
	{
		if (CompletionCallback)
		{
			CompletionCallback(false);
		}
		return;
	}

	FAsyncSaveGameToSlotDelegate Finished;
	Finished.BindLambda([CompletionCallback = MoveTemp(CompletionCallback)](const FString&, const int32, bool bSuccess) mutable
	{
		if (CompletionCallback)
		{
			CompletionCallback(bSuccess);
		}
	});
	UGameplayStatics::AsyncSaveGameToSlot(SaveObject, SlotName, UserIndex, Finished);
}
