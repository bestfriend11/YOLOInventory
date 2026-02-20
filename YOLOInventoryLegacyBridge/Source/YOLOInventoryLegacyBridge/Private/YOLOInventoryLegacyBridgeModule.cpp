#include "Modules/ModuleManager.h"

class FYOLOInventoryLegacyBridgeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FYOLOInventoryLegacyBridgeModule, YOLOInventoryLegacyBridge)
