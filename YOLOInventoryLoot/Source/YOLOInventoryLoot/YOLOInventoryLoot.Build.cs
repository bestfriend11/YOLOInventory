using UnrealBuildTool;

public class YOLOInventoryLoot : ModuleRules
{
    public YOLOInventoryLoot(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "YOLOInventoryCore",
            "YOLOInventorySchema",
            "YOLOInventoryWorld",
            "YOLOInventoryContainers"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            
        });
    }
}
