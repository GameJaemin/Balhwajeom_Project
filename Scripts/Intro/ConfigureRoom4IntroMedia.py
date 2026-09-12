import unreal


def main():
    if not unreal.TabletWidgetBlueprintLibrary.create_intro_flow_assets():
        raise RuntimeError("Failed to update intro UI assets")
    if not unreal.TabletWidgetBlueprintLibrary.configure_room4_intro_media():
        raise RuntimeError("Failed to configure room4 intro media")
    unreal.log("ROOM4_INTRO_MEDIA_SCRIPT Result=Success")


if __name__ == "__main__":
    main()
