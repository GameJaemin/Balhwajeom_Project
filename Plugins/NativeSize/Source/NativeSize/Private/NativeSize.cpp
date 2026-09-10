#include "NativeSize.h"

#include "UMGEditorModule.h"
#include "WidgetBlueprintEditor.h"
#include "WidgetBlueprint.h"

#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FNativeSizeModule"

void FNativeSizeModule::StartupModule()
{
    IUMGEditorModule& UMGEditorModule =
        FModuleManager::LoadModuleChecked<IUMGEditorModule>("UMGEditor");

    UMGEditorModule.AddWidgetEditorToolbarExtender(
        IUMGEditorModule::FWidgetEditorToolbarExtender::CreateRaw(
            this,
            &FNativeSizeModule::ExtendWidgetToolbar
        )
    );
}

void FNativeSizeModule::ShutdownModule()
{
}

TSharedRef<FExtender> FNativeSizeModule::ExtendWidgetToolbar(
    const TSharedRef<FUICommandList> CommandList,
    TSharedRef<FWidgetBlueprintEditor> WidgetEditor)
{
    TSharedRef<FExtender> Extender = MakeShared<FExtender>();

    Extender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        CommandList,
        FToolBarExtensionDelegate::CreateRaw(
            this,
            &FNativeSizeModule::AddToolbarButton,
            TWeakPtr<FWidgetBlueprintEditor>(WidgetEditor)
        )
    );

    return Extender;
}

void FNativeSizeModule::AddToolbarButton(
    FToolBarBuilder& ToolbarBuilder,
    TWeakPtr<FWidgetBlueprintEditor> WidgetEditor)
{
    ToolbarBuilder.AddToolBarButton(
        FUIAction(
            FExecuteAction::CreateRaw(
                this,
                &FNativeSizeModule::ApplyNativeSize,
                WidgetEditor
            )
        ),
        NAME_None,
        LOCTEXT("NativeSizeLabel", "Native Size"),
        LOCTEXT(
            "NativeSizeTooltip",
            "선택한 Image를 원본 이미지 크기로 맞춥니다."
        ),
        FSlateIcon()
    );
}

void FNativeSizeModule::ApplyNativeSize(
    TWeakPtr<FWidgetBlueprintEditor> WidgetEditorWeak)
{
    TSharedPtr<FWidgetBlueprintEditor> WidgetEditor = WidgetEditorWeak.Pin();

    if (!WidgetEditor.IsValid())
    {
        return;
    }

    const TSet<FWidgetReference>& SelectedWidgets =
        WidgetEditor->GetSelectedWidgets();

    if (SelectedWidgets.Num() == 0)
    {
        return;
    }

    const FScopedTransaction Transaction(
        LOCTEXT("NativeSizeTransaction", "Set Image Native Size")
    );

    bool bChangedAnything = false;

    for (const FWidgetReference& WidgetReference : SelectedWidgets)
    {
        UWidget* Widget = WidgetReference.GetTemplate();

        UImage* Image = Cast<UImage>(Widget);

        if (!Image)
        {
            continue;
        }

        UObject* ResourceObject =
            Image->GetBrush().GetResourceObject();

        UTexture2D* Texture =
            Cast<UTexture2D>(ResourceObject);

        if (!Texture)
        {
            continue;
        }

        const FVector2D TextureSize(
            Texture->GetSizeX(),
            Texture->GetSizeY()
        );

        Image->Modify();

        // 이미지가 요구하는 원본 크기 설정
        Image->SetDesiredSizeOverride(TextureSize);

        // Canvas Panel 안에 있는 이미지라면
        // 실제 캔버스 슬롯 크기도 같이 변경
        if (UCanvasPanelSlot* CanvasSlot =
            Cast<UCanvasPanelSlot>(Image->Slot))
        {
            CanvasSlot->Modify();
            CanvasSlot->SetSize(TextureSize);
        }

        bChangedAnything = true;
    }

    if (bChangedAnything)
    {
        UWidgetBlueprint* WidgetBlueprint =
            WidgetEditor->GetWidgetBlueprintObj();

        if (WidgetBlueprint)
        {
            WidgetBlueprint->Modify();
            WidgetBlueprint->MarkPackageDirty();
        }

        WidgetEditor->InvalidatePreview(false);
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNativeSizeModule, NativeSize)