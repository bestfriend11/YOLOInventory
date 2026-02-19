using UnrealBuildTool;

public class YOLOInventoryContainers : ModuleRules
{
    public YOLOInventoryContainers(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings",
            "GameplayAbilities",
            "GameplayTasks",
            "NetCore",
            "Slate",
            "SlateCore",
            "UMG",
            "YOLOInventoryCore",
            "YOLOInventorySchema"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "InputCore",
            "Networking",
            "RenderCore"
        });
    }
}
