using UnrealBuildTool;

public class ItemInspectorRuntime : ModuleRules
{
	public ItemInspectorRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings",
			"GameplayTags",
			"InputCore",
			"SlateCore",
			"UMG",
			"JMInteraction",
			"JMGameplayEvent"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate"
		});
	}
}
