using UnrealBuildTool;

public class YOLOInventory : ModuleRules
{
    public YOLOInventory(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "GameplayTags",
            "GameplayAbilities",
            "PhysicsCore",
            "CSVItemBuilder",
            "YOLOInventoryCore",
            "YOLOInventorySchema"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AssetRegistry",
            "RenderCore",
            "Projects"
        });
    }
}
