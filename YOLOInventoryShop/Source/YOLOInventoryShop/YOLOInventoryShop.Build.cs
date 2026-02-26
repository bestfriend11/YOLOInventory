using UnrealBuildTool;

public class YOLOInventoryShop : ModuleRules
{
    public YOLOInventoryShop(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "YOLOInventoryCore",
            "YOLOInventorySchema",
            "YOLOInventoryContainers"
        });
    }
}
