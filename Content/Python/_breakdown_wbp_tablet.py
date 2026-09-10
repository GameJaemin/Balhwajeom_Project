import unreal
if not unreal.TabletWidgetBlueprintLibrary.inspect_widget_blueprint_by_path("/Game/Balhwajeom/UI/Tablet/WBP_Tablet"):
    raise RuntimeError("failed")
