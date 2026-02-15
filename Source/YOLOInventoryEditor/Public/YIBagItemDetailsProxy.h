#pragma once

#include "CoreMinimal.h"
#include "YIInventoryBag.h"
#include "UObject/Object.h"
#include "YIBagItemDetailsProxy.generated.h"

class UYIInventoryBag;

/**
 * Transient details object used by bag dashboard to inspect a selected bag item instance.
 */
UCLASS(Transient)
class YOLOINVENTORYEDITOR_API UYIBagItemDetailsProxy : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, Category = "Bag Item")
	TSoftObjectPtr<UYIInventoryBag> SourceBag;

	UPROPERTY(VisibleAnywhere, Category = "Bag Item")
	int32 ItemIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, Category = "Bag Item")
	FYIBagItem ItemInstance;

	void LoadFromBag(UYIInventoryBag* InBag, int32 InItemIndex);
};
