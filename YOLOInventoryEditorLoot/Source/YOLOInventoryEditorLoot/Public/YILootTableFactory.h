#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YILootTableFactory.generated.h"

UCLASS()
class YOLOINVENTORYEDITORLOOT_API UYILootTableFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYILootTableFactory();
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
