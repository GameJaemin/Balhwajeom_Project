"""Point WBP_CAM at its runtime class so the zoom bar can be driven from C++.

WBP_CAM is a designer-owned asset: its layout, the camera frame and the Bar image are all
authored in the Widget Blueprint. Only the parent class is set here, which is what gives
UBalhwajeomCameraViewfinderWidget its BindWidgetOptional access to Bar.

Run through UnrealEditor-Cmd with ``-ExecutePythonScript``. The script is idempotent, so
it can be re-run after WBP_CAM is merged from another branch and lost its parent.
"""

import unreal


WIDGET_PATH = "/Game/Balhwajeom/UI/HUD/WBP_CAM"
PARENT_CLASS_PATH = "/Script/Balhwajeom.BalhwajeomCameraViewfinderWidget"
BAR_WIDGET_NAME = "Bar"


def main():
    blueprint = unreal.load_asset(WIDGET_PATH)
    if blueprint is None:
        raise RuntimeError(f"Could not load {WIDGET_PATH}")

    parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
    if parent_class is None:
        raise RuntimeError(
            f"Could not load {PARENT_CLASS_PATH}. Build BalhwajeomEditor first."
        )

    # Checked through the CDO because a Widget Blueprint does not expose ParentClass to
    # Python, and re-running a reparent that already happened would dirty the asset for
    # nothing.
    generated = blueprint.generated_class()
    if generated is not None and isinstance(
        unreal.get_default_object(generated),
        unreal.BalhwajeomCameraViewfinderWidget,
    ):
        unreal.log(
            f"CAMERA_VIEWFINDER_WIDGET Result=Skipped ({WIDGET_PATH} already derives "
            f"from {parent_class.get_name()})")
        return

    unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, parent_class)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {WIDGET_PATH}")

    # The reparent is only useful if the authored Bar survived it.
    if not unreal.TabletWidgetBlueprintLibrary.inspect_widget_blueprint_by_path(
        f"{WIDGET_PATH}.{blueprint.get_name()}"
    ):
        raise RuntimeError(f"{WIDGET_PATH} failed inspection after reparenting")

    unreal.log(
        f"CAMERA_VIEWFINDER_WIDGET Result=Success Parent={parent_class.get_name()} "
        f"Bar={BAR_WIDGET_NAME}")


main()
