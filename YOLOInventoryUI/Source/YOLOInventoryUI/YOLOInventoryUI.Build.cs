using UnrealBuildTool;

public class YOLOInventoryUI : ModuleRules
{
    public YOLOInventoryUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
            "Slate",
            "SlateCore",
            "EnhancedInput",
            "GameplayTags",
            "GameplayAbilities",
            "UILayered",
            "InputCore",
            "YOLOInventoryCore",
            "YOLOInventorySchema",
            "YOLOInventoryGrid",
            "YOLOInventoryEquipment",
            "YOLOInventoryShop",
            "YOLOInventoryTrade",
            "YOLOInventoryWorld",
            "YOLOInventoryContainers"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "ApplicationCore"
        });
    }
}
