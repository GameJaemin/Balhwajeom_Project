#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FWidgetBlueprintEditor;
class FExtender;
class FUICommandList;

class FNativeSizeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedRef<FExtender> ExtendWidgetToolbar(
        const TSharedRef<FUICommandList> CommandList,
        TSharedRef<FWidgetBlueprintEditor> WidgetEditor);

    void AddToolbarButton(
        class FToolBarBuilder& ToolbarBuilder,
        TWeakPtr<FWidgetBlueprintEditor> WidgetEditor);

    void ApplyNativeSize(TWeakPtr<FWidgetBlueprintEditor> WidgetEditor);
};