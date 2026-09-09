import unreal


ASSET_PATH = "/Game/Balhwajeom/Gameplay/Interaction/BedMemory/BP_BedMemory"
PACKAGE_PATH = "/Game/Balhwajeom/Gameplay/Interaction/BedMemory"
ASSET_NAME = "BP_BedMemory"


def create_or_validate_blueprint():
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        blueprint = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
        if not isinstance(blueprint, unreal.Blueprint):
            raise RuntimeError(f"{ASSET_PATH} exists but is not a Blueprint")
        unreal.log(f"Using existing asset: {ASSET_PATH}")
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.BedMemoryActor)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME,
            PACKAGE_PATH,
            unreal.Blueprint,
            factory,
        )
        if blueprint is None:
            raise RuntimeError(f"Failed to create {ASSET_PATH}")
        unreal.log(f"Created asset: {ASSET_PATH}")

    blueprint.set_editor_property(
        "blueprint_description",
        "Photo-memory bed interaction. Assign BedMesh, optional animation montages, BGM, attenuation, and PhotoEmitterMap in this Blueprint.",
    )
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save {ASSET_PATH}")

    generated_class = unreal.EditorAssetLibrary.load_blueprint_class(ASSET_PATH)
    if generated_class is None:
        raise RuntimeError("BP_BedMemory generated class could not be loaded")
    default_object = unreal.get_default_object(generated_class)
    required_components = (
        "player_anchor",
        "exit_anchor",
        "seated_camera",
        "voice_player",
        "voice_origin",
        "voice_desk",
        "voice_door",
        "voice_bed",
        "voice_hall",
        "voice_photo",
    )
    for property_name in required_components:
        if default_object.get_editor_property(property_name) is None:
            raise RuntimeError(f"BP_BedMemory is missing {property_name}")
    bed_mesh = default_object.get_editor_property("bed_mesh")
    assigned_mesh = bed_mesh.get_editor_property("static_mesh")
    if unreal.SystemLibrary.is_valid(assigned_mesh):
        unreal.log(f"BP_BedMemory preserves assigned BedMesh: {assigned_mesh.get_path_name()}")

    unreal.log("BP_BedMemory is compiled, saved, and component validation passed.")


create_or_validate_blueprint()
