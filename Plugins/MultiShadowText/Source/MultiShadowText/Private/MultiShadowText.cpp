#include "MultiShadowText.h"
#include "Modules/ModuleManager.h"

class FMultiShadowTextModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FMultiShadowTextModule, MultiShadowText)