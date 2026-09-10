import unreal

if not unreal.TabletWidgetBlueprintLibrary.inspect_widget_blueprint_by_path("/Game/Balhwajeom/UI/JE/WBP_Layout"):
    raise RuntimeError("Failed to inspect /Game/Balhwajeom/UI/JE/WBP_Layout")
