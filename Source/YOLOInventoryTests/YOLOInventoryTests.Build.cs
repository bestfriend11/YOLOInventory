using UnrealBuildTool;

public class YOLOInventoryTests : ModuleRules
{
    public YOLOInventoryTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "YOLOInventory",
            "UnrealEd"
        });
    }
}
