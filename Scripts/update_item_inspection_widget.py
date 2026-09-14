import unreal


if not unreal.TabletWidgetBlueprintLibrary.center_item_inspection_widget():
    raise RuntimeError("Failed to rebuild WBP_JMItemInspection")

unreal.log("ITEM_INSPECTION_WIDGET_UPDATE Result=Success")
