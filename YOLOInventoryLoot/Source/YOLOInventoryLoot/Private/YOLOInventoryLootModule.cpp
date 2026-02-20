#include "Modules/ModuleManager.h"

class FYOLOInventoryLootModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FYOLOInventoryLootModule, YOLOInventoryLoot)
