import unreal


def main():
    if not unreal.TabletWidgetBlueprintLibrary.create_capture_photo_widget_blueprint():
        raise RuntimeError("Failed to create WBP_CapturePhoto")
    if not unreal.TabletWidgetBlueprintLibrary.inspect_widget_blueprint_by_path(
        "/Game/Balhwajeom/UI/Camera/WBP_CapturePhoto.WBP_CapturePhoto"
    ):
        raise RuntimeError("Failed to inspect WBP_CapturePhoto")
    unreal.log("CAPTURE_PHOTO_WIDGET_CREATE Result=Success")


if __name__ == "__main__":
    main()
