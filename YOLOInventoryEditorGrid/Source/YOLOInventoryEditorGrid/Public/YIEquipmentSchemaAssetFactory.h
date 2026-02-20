#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIEquipmentSchemaAssetFactory.generated.h"

UCLASS()
class YOLOINVENTORYEDITORGRID_API UYIEquipmentSchemaAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIEquipmentSchemaAssetFactory();
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

