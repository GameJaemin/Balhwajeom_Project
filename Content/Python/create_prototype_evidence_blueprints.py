import unreal


MAP_PATH = "/Game/Balhwajeom/Maps/Prototype/L_InvestigationPrototype"
BLUEPRINT_FOLDER = "/Game/Balhwajeom/Blueprints/Investigation/Evidence"

SPECS = (
    {
        "label": "Evidence_PigMirror",
        "asset_name": "BP_Evidence_PigMirror",
        "object_id": "OBJ_PIG_MIRROR",
    },
    {
        "label": "Evidence_SnowGlobe",
        "asset_name": "BP_Evidence_SnowGlobe",
        "object_id": "OBJ_SNOW_GLOBE",
    },
)

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"Could not load {MAP_PATH}")

unreal.EditorAssetLibrary.make_directory(BLUEPRINT_FOLDER)
actors_by_label = {
    actor.get_actor_label(): actor for actor in actor_subsystem.get_all_level_actors()
}


def create_or_update_blueprint(spec, source_actor):
    asset_path = f"{BLUEPRINT_FOLDER}/{spec['asset_name']}"
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not blueprint:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.BalhwajeomEvidenceActor)
        blueprint = asset_tools.create_asset(
            spec["asset_name"], BLUEPRINT_FOLDER, unreal.Blueprint, factory
        )
    if not blueprint:
        raise RuntimeError(f"Could not create {asset_path}")

    generated_class = blueprint.generated_class()
    default_actor = unreal.get_default_object(generated_class)
    default_actor.configure_investigation_object(spec["object_id"])

    source_mesh = source_actor.get_component_by_class(unreal.StaticMeshComponent)
    default_mesh = default_actor.get_component_by_class(unreal.StaticMeshComponent)
    if not source_mesh or not default_mesh:
        raise RuntimeError(f"Missing evidence mesh for {spec['label']}")
    default_mesh.set_static_mesh(source_mesh.get_editor_property("static_mesh"))
    default_mesh.set_editor_property(
        "relative_scale3d", source_actor.get_actor_scale3d()
    )

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
        raise RuntimeError(f"Could not save {asset_path}")
    return blueprint.generated_class(), asset_path


replacements = []
for spec in SPECS:
    source = actors_by_label.get(spec["label"])
    if not source:
        raise RuntimeError(f"Missing source actor: {spec['label']}")

    source_location = source.get_actor_location()
    source_rotation = source.get_actor_rotation()
    source_scale = source.get_actor_scale3d()
    source_mesh = source.get_component_by_class(unreal.StaticMeshComponent)
    source_mesh_asset = source_mesh.get_editor_property("static_mesh") if source_mesh else None
    source_tags = list(source.get_editor_property("tags"))
    source_folder = source.get_folder_path()

    blueprint_class, asset_path = create_or_update_blueprint(spec, source)
    replacement = actor_subsystem.spawn_actor_from_class(
        blueprint_class,
        source_location,
        source_rotation,
    )
    if not replacement:
        raise RuntimeError(f"Could not spawn {asset_path}")

    replacement.set_actor_scale3d(source_scale)
    replacement.set_actor_label(spec["label"])
    replacement.configure_investigation_object(spec["object_id"])
    replacement.set_editor_property("tags", source_tags)
    replacement.set_folder_path(source_folder)

    replacement_mesh = replacement.get_component_by_class(unreal.StaticMeshComponent)
    if replacement_mesh and source_mesh_asset:
        replacement_mesh.set_static_mesh(source_mesh_asset)

    replacements.append((source, replacement, spec, asset_path))

for source, replacement, spec, asset_path in replacements:
    actor_subsystem.destroy_actor(source)
    unreal.log(
        f"PROTOTYPE_EVIDENCE_BLUEPRINT Label={spec['label']} Asset={asset_path} "
        f"Class={replacement.get_class().get_path_name()} ObjectID={replacement.get_object_id()} "
        f"Location={replacement.get_actor_location()} Rotation={replacement.get_actor_rotation()} "
        f"Scale={replacement.get_actor_scale3d()}"
    )

if not level_subsystem.save_current_level():
    raise RuntimeError(f"Could not save {MAP_PATH}")

unreal.log("PROTOTYPE_EVIDENCE_BLUEPRINT_CONVERSION Result=Success Count=2")
