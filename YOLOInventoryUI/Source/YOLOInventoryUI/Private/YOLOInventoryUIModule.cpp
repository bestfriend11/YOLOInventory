#include "Modules/ModuleManager.h"

class FYOLOInventoryUIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FYOLOInventoryUIModule, YOLOInventoryUI)
