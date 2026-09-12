using UnrealBuildTool;

public class JMGameplayEventTests : ModuleRules
{
    public JMGameplayEventTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "GameplayTags", "JMGameplayEvent"
        });
    }
}
