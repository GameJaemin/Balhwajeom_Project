import unreal


def main():
    if not unreal.TabletWidgetBlueprintLibrary.redesign_messenger_widget():
        raise RuntimeError("Failed to rebuild WBP_Messenger")

    unreal.log("WBP_Messenger rebuilt as the static four-room messenger UI.")


main()
