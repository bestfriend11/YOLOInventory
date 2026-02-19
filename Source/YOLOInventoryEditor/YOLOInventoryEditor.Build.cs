using UnrealBuildTool;

public class YOLOInventoryEditor : ModuleRules
{
    public YOLOInventoryEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "EditorFramework",
            "UnrealEd",
            "GraphEditor",
            "Kismet",
            "KismetWidgets",
            "PropertyEditor",
            "AssetTools",
            "ToolMenus",
            "Projects",
            "ContentBrowser", "AssetRegistry",
            "EditorWidgets",
            "PropertyEditor",
            "InputCore",
            "AppFramework",
            "EditorStyle",
            "ClassViewer",
            "Niagara",
            "GameplayTags",
            "DataTableEditor",
            "CSVItemBuilder",
            "YOLOInventoryEditorCore"
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "YOLOInventoryContainers",
            "YOLOInventory",
            "YOLOInventoryWorld"
        });
    }
}
