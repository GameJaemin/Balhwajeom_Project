import unreal


if not unreal.TabletWidgetBlueprintLibrary.upgrade_investigation_data_tables():
    raise RuntimeError("Failed to upgrade investigation DataTables")
