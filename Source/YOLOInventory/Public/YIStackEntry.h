#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YIInventoryTypes.h"
#include "YIStackEntry.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORY_API UYIStackEntry : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stack")
	FText DisplayName;

	// Rarity that can be used by UI and gameplay; used in editor preview color coding
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stack")
	EYOLOItemRarity Rarity = EYOLOItemRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stack")
	bool bEnabled = true;
};

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORY_API UYIStackEntry_UI : public UYIStackEntry
{
	GENERATED_BODY()
};

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORY_API UYIStackEntry_Ability : public UYIStackEntry
{
	GENERATED_BODY()
};

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORY_API UYIStackEntry_Upgrade : public UYIStackEntry
{
	GENERATED_BODY()
};

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORY_API UYIStackEntry_Economy : public UYIStackEntry
{
	GENERATED_BODY()
};
