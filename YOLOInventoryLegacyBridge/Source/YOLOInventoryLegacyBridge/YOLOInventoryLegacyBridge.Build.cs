using UnrealBuildTool;

public class YOLOInventoryLegacyBridge : ModuleRules
{
    public YOLOInventoryLegacyBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "PhysicsCore",
            "YOLOInventoryContainers",
            "YOLOInventoryCore",
            "YOLOInventorySchema",
            "YOLOInventoryEquipment"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            
        });
    }
}
