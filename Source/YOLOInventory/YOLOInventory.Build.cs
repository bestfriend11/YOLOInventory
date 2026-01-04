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
            "EnhancedInput",
            "NiagaraCore", "GameplayAbilities", "GameplayTasks", "Slate", "SlateCore", "UILayered"
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
			"InputCore"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd"
            });
        }
    }
}
