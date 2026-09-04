import unreal


if not unreal.TabletWidgetBlueprintLibrary.redesign_tablet_widget_blueprint():
    raise RuntimeError("Failed to redesign /Game/Balhwajeom/UI/Tablet/WBP_Tablet")
