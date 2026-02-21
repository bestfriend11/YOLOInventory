using UnrealBuildTool;

public class YOLOInventoryEditorSchema : ModuleRules
{
    public YOLOInventoryEditorSchema(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "YOLOInventorySchema",
            "YOLOInventoryLegacyBridge",
            "YOLOInventoryContainers",
            "YOLOInventoryEditorCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "InputCore",
            "AppFramework",
            "EditorStyle",
            "UnrealEd",
            "EditorFramework",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "PropertyEditor",
            "AssetTools",
            "ContentBrowser",
            "AssetRegistry",
            "DataTableEditor",
            "GameplayTags",
            "BlueprintGraph",
            "Kismet",
            "CSVItemBuilder",
            "YOLOInventoryEquipment"
        });
    }
}
