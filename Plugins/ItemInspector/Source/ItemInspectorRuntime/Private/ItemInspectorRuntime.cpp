#include "ItemInspectorRuntime.h"

#include "Core/JMGameplayLog.h"

#define LOCTEXT_NAMESPACE "FItemInspectorRuntimeModule"

void FItemInspectorRuntimeModule::StartupModule()
{
	UE_LOG(LogJMGameplay, Log, TEXT("ItemInspectorRuntime module started."));
}

void FItemInspectorRuntimeModule::ShutdownModule()
{
	UE_LOG(LogJMGameplay, Log, TEXT("ItemInspectorRuntime module shut down."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FItemInspectorRuntimeModule, ItemInspectorRuntime)
