import unreal


if not unreal.TabletWidgetBlueprintLibrary.inspect_tablet_widget_blueprint():
    raise RuntimeError("Failed to inspect /Game/Balhwajeom/UI/Tablet/WBP_Tablet")
