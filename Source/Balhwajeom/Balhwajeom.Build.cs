// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Balhwajeom : ModuleRules
{
	public Balhwajeom(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Balhwajeom",
			"Balhwajeom/Variant_Platforming",
			"Balhwajeom/Variant_Platforming/Animation",
			"Balhwajeom/Variant_Combat",
			"Balhwajeom/Variant_Combat/AI",
			"Balhwajeom/Variant_Combat/Animation",
			"Balhwajeom/Variant_Combat/Gameplay",
			"Balhwajeom/Variant_Combat/Interfaces",
			"Balhwajeom/Variant_Combat/UI",
			"Balhwajeom/Variant_SideScrolling",
			"Balhwajeom/Variant_SideScrolling/AI",
			"Balhwajeom/Variant_SideScrolling/Gameplay",
			"Balhwajeom/Variant_SideScrolling/Interfaces",
			"Balhwajeom/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
