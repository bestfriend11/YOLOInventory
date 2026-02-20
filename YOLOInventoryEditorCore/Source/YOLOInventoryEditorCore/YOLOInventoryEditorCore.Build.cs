using UnrealBuildTool;

public class YOLOInventoryEditorCore : ModuleRules
{
    public YOLOInventoryEditorCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "YOLOInventoryCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UnrealEd",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "PropertyEditor",
            "AssetRegistry",
            "AssetTools",
            "EditorFramework",
            "Kismet"
        });
    }
}
