"""Build WBP_TutorialOverlay's authored layout.

Run through UnrealEditor-Cmd with ``-ExecutePythonScript`` after compiling
UBalhwajeomTutorialOverlayWidget. The script is idempotent: it reparents the
asset onto the runtime class, clears whatever tree is there and rebuilds it, so
it can be re-run after the widget is merged from another branch.

Only the layout is authored here. Which row of DT_TutorialOverlay is shown, and
when, belongs to the presenter.
"""

import unreal


ASSET_PATH = "/Game/Balhwajeom/UI/HUD/WBP_TutorialOverlay.WBP_TutorialOverlay"


def main():
    if not unreal.TabletWidgetBlueprintLibrary.create_tutorial_overlay_widget_blueprint():
        raise RuntimeError("Failed to build WBP_TutorialOverlay")
    if not unreal.TabletWidgetBlueprintLibrary.inspect_widget_blueprint_by_path(ASSET_PATH):
        raise RuntimeError("Failed to inspect WBP_TutorialOverlay")
    unreal.log("TUTORIAL_OVERLAY_WIDGET_CREATE Result=Success")


if __name__ == "__main__":
    main()
