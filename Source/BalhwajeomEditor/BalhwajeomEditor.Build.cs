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
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AssetRegistry",
			"AssetTools",
			"AnimGraph",
			"Balhwajeom",
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
