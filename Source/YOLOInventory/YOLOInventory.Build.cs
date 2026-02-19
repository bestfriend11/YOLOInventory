using UnrealBuildTool;

public class YOLOInventory : ModuleRules
{
    public YOLOInventory(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "Niagara",
            "PhysicsCore",
            "GameplayTags",
            "NiagaraCore", "GameplayAbilities", "GameplayTasks", "Slate", "SlateCore",
            "CSVItemBuilder",
            "YOLOInventoryCore",
            "YOLOInventorySchema",
            "YOLOInventoryEquipment"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "ToolMenus"
            });
        }

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
			"UMG",
			"InputCore",
            "NetCore",
            "Networking",
            "RenderCore",
            "Projects"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd"
            });
        }

        CircularlyReferencedDependentModules.AddRange(new string[]
        {
            "YOLOInventoryEquipment"
        });
    }
}
