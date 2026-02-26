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
            "GameplayAbilities",
            "YOLOInventoryCore",
            "YOLOInventorySchema"
        });
    }
}
