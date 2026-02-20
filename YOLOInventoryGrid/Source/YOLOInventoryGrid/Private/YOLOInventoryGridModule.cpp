#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

class FYOLOInventoryGridModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("YOLOInventoryGrid")))
        {
            const FString ShaderDirectory = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
            // New canonical virtual root for suite grid shaders.
            AddShaderSourceDirectoryMapping(TEXT("/Plugin/YOLOInventoryGrid"), ShaderDirectory);
            // Legacy compatibility root so existing materials/functions keep compiling.
            AddShaderSourceDirectoryMapping(TEXT("/Plugin/YOLOInventory"), ShaderDirectory);
        }
    }

    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FYOLOInventoryGridModule, YOLOInventoryGrid)
