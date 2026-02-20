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
            "SlateCore",
            "Slate"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects",
            "RenderCore"
        });
    }
}
