using UnrealBuildTool;

public class YOLOInventoryTrade : ModuleRules
{
    public YOLOInventoryTrade(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "UMG",
            "YOLOInventoryCore",
            "YOLOInventorySchema",
            "YOLOInventoryContainers"
        });
    }
}
