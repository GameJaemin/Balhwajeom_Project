import unreal

WBP_TABLET_PATH = "/Game/Balhwajeom/UI/Tablet/WBP_Tablet"
ICON_TEXTURE_PATH = "/Game/Balhwajeom/UI/Tablet/Folder.Folder"

blueprint = unreal.load_asset(WBP_TABLET_PATH)
if not blueprint:
    raise RuntimeError(f"Could not load {WBP_TABLET_PATH}")

texture = unreal.load_asset(ICON_TEXTURE_PATH)
if not texture:
    raise RuntimeError(f"Could not load {ICON_TEXTURE_PATH}")

generated_class = blueprint.generated_class()
if not generated_class:
    raise RuntimeError(f"{WBP_TABLET_PATH} has no generated class")

default_object = unreal.get_default_object(generated_class)
default_object.set_editor_property("default_folder_icon", texture)

if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint):
    raise RuntimeError(f"Could not save {WBP_TABLET_PATH}")

unreal.log("SET_FOLDER_ICON Result=Success")
