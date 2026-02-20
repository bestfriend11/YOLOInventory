#include "Modules/ModuleManager.h"

class FYOLOInventoryCoreModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FYOLOInventoryCoreModule, YOLOInventoryCore)
