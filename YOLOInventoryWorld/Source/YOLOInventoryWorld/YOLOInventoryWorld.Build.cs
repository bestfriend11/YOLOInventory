using UnrealBuildTool;

public class YOLOInventoryWorld : ModuleRules
{
    public YOLOInventoryWorld(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "GameplayTags",
            "YOLOInventoryCore",
            "YOLOInventorySchema",
            "YOLOInventoryContainers"
        });
    }
}
