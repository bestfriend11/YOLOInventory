using UnrealBuildTool;

public class YOLOInventoryGrid : ModuleRules
{
    public YOLOInventoryGrid(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
            "SlateCore",
            "Slate",
            "InputCore",
            "GameplayTags",
            "GameplayAbilities",
            "YOLOInventoryCore",
            "YOLOInventorySchema",
            "YOLOInventoryContainers",
            "StructUtils"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects",
            "RenderCore",
            "ApplicationCore"
        });
    }
}
