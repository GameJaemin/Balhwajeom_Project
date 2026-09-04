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
			"Balhwajeom",
			"SlateCore",
			"UMG",
			"UMGEditor",
			"UnrealEd"
		});
	}
}
