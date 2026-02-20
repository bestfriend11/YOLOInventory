using UnrealBuildTool;

public class YOLOInventoryEditorGrid : ModuleRules
{
    public YOLOInventoryEditorGrid(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "YOLOInventorySchema",
            "YOLOInventoryContainers",
            "YOLOInventoryEquipment",
            "YOLOInventoryGrid",
            "YOLOInventoryEditorCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "InputCore",
            "UnrealEd",
            "EditorFramework",
            "Slate",
            "SlateCore",
            "AppFramework",
            "EditorStyle",
            "ToolMenus",
            "PropertyEditor",
            "AssetTools",
            "ContentBrowser",
            "AssetRegistry",
            "CSVItemBuilder"
        });
    }
}
