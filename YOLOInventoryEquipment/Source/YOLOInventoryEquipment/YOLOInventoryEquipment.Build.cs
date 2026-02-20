using UnrealBuildTool;

public class YOLOInventoryEquipment : ModuleRules
{
    public YOLOInventoryEquipment(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "GameplayAbilities",
            "GameplayTasks",
            "YOLOInventoryCore",
            "YOLOInventorySchema",
            "YOLOInventoryContainers"
        });
    }
}
