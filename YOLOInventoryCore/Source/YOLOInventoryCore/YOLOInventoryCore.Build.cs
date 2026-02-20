using UnrealBuildTool;

public class YOLOInventoryCore : ModuleRules
{
    public YOLOInventoryCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "NetCore",
            "DeveloperSettings",
            "GameplayTags"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            
        });
    }
}
