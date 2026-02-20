#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIRarityProfileFactory.generated.h"

UCLASS()
class YOLOINVENTORYEDITORLOOT_API UYIRarityProfileFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIRarityProfileFactory();
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
