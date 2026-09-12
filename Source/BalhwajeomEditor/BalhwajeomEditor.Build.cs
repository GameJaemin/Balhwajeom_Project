using UnrealBuildTool;

public class BalhwajeomEditor : ModuleRules
{
	public BalhwajeomEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AssetRegistry",
			"AssetTools",
			"AnimGraph",
			"Balhwajeom",
			"ItemInspectorRuntime",
			"BlueprintGraph",
			"ImageCore",
			"SlateCore",
			"Slate",
			"RenderCore",
			"UMG",
			"UMGEditor",
			"UnrealEd"
		});
	}
}
