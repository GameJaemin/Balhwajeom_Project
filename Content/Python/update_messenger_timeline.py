import unreal

if not unreal.TabletWidgetBlueprintLibrary.update_messenger_timeline():
    raise RuntimeError("Messenger timeline update failed")
if not unreal.TabletWidgetBlueprintLibrary.test_messenger_timeline():
    raise RuntimeError("Messenger timeline test failed")
