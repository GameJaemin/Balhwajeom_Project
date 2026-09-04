import unreal


if not unreal.TabletWidgetBlueprintLibrary.create_messenger_data_assets():
    raise RuntimeError("Failed to create messenger catalog/room Data Assets")
