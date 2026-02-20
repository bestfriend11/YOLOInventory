using UnrealBuildTool;

public class YOLOInventoryTemplateDS1 : ModuleRules
{
    public YOLOInventoryTemplateDS1(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "YOLOInventoryCore",
            "YOLOInventorySchema",
            "YOLOInventoryGrid",
            "YOLOInventoryEquipment",
            "YOLOInventoryLoot",
            "YOLOInventoryUI"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            
        });
    }
}
