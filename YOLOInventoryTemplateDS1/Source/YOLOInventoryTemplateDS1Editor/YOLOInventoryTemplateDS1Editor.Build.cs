using UnrealBuildTool;

public class YOLOInventoryTemplateDS1Editor : ModuleRules
{
    public YOLOInventoryTemplateDS1Editor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "YOLOInventorySchema",
            "YOLOInventoryLoot",
            "YOLOInventoryContainers"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UnrealEd",
            "AssetRegistry",
            "AssetTools"
        });
    }
}
