using UnrealBuildTool;

public class JMInteraction : ModuleRules
{
    public JMInteraction(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "DeveloperSettings", "GameplayTags", "Slate", "SlateCore", "UMG", "JMGameplayEvent"
        });

    }
}
