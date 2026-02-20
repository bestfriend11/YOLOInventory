#include "YIInventoryBagFactory.h"
#include "YIInventoryBag.h"
#include "YIEditorGridCategory.h"

UYIInventoryBagFactory::UYIInventoryBagFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UYIInventoryBag::StaticClass();
}

UObject* UYIInventoryBagFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIInventoryBag>(InParent, Class, Name, Flags);
}

uint32 UYIInventoryBagFactory::GetMenuCategories() const
{
	return YIEditorGridCategory::Get();
}
