using UnrealBuildTool;

public class YOLOInventoryEditorLoot : ModuleRules
{
    public YOLOInventoryEditorLoot(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "YOLOInventorySchema",
            "YOLOInventoryContainers",
            "YOLOInventoryLoot",
            "YOLOInventoryEditorCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "InputCore",
            "UnrealEd",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "PropertyEditor",
            "AssetTools",
            "ContentBrowser",
            "AssetRegistry"
        });
    }
}
