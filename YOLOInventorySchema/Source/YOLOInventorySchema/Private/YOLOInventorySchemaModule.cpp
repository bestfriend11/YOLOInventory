#include "Modules/ModuleManager.h"

class FYOLOInventorySchemaModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FYOLOInventorySchemaModule, YOLOInventorySchema)
