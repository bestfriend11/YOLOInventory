#include "Modules/ModuleManager.h"

class FYOLOInventoryTemplateDS1EditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FYOLOInventoryTemplateDS1EditorModule, YOLOInventoryTemplateDS1Editor)
