import unreal


def main():
    if not unreal.TabletWidgetBlueprintLibrary.configure_object_label_layout():
        raise RuntimeError("Failed to configure WBP_ObjectLabel layout")
    if not unreal.TabletWidgetBlueprintLibrary.inspect_widget_blueprint_by_path(
        "/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel.WBP_ObjectLabel"
    ):
        raise RuntimeError("Failed to inspect WBP_ObjectLabel")
    unreal.log("OBJECT_LABEL_LAYOUT_CONFIGURE Result=Success")


if __name__ == "__main__":
    main()
