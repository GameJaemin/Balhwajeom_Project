using UnrealBuildTool;

public class ItemInspectorTests : ModuleRules
{
	public ItemInspectorTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ItemInspectorRuntime",
			"SlateCore",
			"UMG",
			"UnrealEd"
		});
	}
}
