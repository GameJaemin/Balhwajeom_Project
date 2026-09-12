import unreal


def main():
    if not unreal.TabletWidgetBlueprintLibrary.create_intro_flow_assets():
        raise RuntimeError("Failed to create intro flow assets")

    unreal.log("INTRO_FLOW_ASSET_CREATE Result=Success")


if __name__ == "__main__":
    main()
