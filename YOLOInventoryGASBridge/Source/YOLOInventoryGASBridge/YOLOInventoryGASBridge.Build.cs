using UnrealBuildTool;

public class YOLOInventoryGASBridge : ModuleRules
{
    public YOLOInventoryGASBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayAbilities",
            "GameplayTags",
            "YOLOInventoryCore",
            "YOLOInventorySchema"
        });
    }
}

