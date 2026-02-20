#include "Modules/ModuleManager.h"

class FYOLOInventoryEquipmentModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FYOLOInventoryEquipmentModule, YOLOInventoryEquipment)
