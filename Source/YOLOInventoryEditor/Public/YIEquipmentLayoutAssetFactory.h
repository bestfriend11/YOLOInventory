#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIEquipmentLayoutAssetFactory.generated.h"

UCLASS()
class YOLOINVENTORYEDITOR_API UYIEquipmentLayoutAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIEquipmentLayoutAssetFactory();
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

