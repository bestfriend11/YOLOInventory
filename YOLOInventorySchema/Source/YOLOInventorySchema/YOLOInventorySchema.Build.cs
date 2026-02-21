using UnrealBuildTool;

public class YOLOInventorySchema : ModuleRules
{
    public YOLOInventorySchema(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "StructUtils",
            "GameplayTags",
            "GameplayAbilities",
            "CSVItemBuilder",
            "YOLOInventoryCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AssetRegistry"
        });
    }
}
