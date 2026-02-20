#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIRarityPaletteFactory.generated.h"

UCLASS()
class UYIRarityPaletteFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIRarityPaletteFactory();
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLORarityPaletteFactory", "FactoryDisplayName", "YOLO Rarity Palette"); }
};
