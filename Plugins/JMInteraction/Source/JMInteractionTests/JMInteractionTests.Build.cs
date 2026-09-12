using UnrealBuildTool;

public class JMInteractionTests : ModuleRules
{
    public JMInteractionTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "JMInteraction", "SlateCore", "UMG"
        });
    }
}
