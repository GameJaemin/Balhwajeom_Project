using UnrealBuildTool;

public class JMGameplayEvent : ModuleRules
{
    public JMGameplayEvent(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "GameplayTags", "DeveloperSettings"
        });
    }
}
