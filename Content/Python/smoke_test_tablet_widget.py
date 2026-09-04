import unreal


if not unreal.TabletWidgetBlueprintLibrary.run_tablet_widget_smoke_test():
    raise RuntimeError("WBP_Tablet smoke test failed")
