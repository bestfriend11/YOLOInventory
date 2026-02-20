#include "Modules/ModuleManager.h"

class FYOLOInventoryTemplateDS1Module : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FYOLOInventoryTemplateDS1Module, YOLOInventoryTemplateDS1)
