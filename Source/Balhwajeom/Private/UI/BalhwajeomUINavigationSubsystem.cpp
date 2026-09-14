#include "UI/BalhwajeomUINavigationSubsystem.h"

#include "Framework/Application/NavigationConfig.h"
#include "Framework/Application/SlateApplication.h"


namespace
{
	/** Same as the default Slate navigation config, minus Tab / Shift+Tab focus cycling. */
	class FBalhwajeomTabletNavigationConfig : public FNavigationConfig
	{
	public:
		virtual EUINavigation GetNavigationDirectionFromKey(
			const FKeyEvent& InKeyEvent) const override
		{
			if (InKeyEvent.GetKey() == EKeys::Tab)
			{
				return EUINavigation::Invalid;
			}

			return FNavigationConfig::GetNavigationDirectionFromKey(InKeyEvent);
		}
	};
}


void UBalhwajeomUINavigationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetNavigationConfig(MakeShared<FBalhwajeomTabletNavigationConfig>());
	}
}
