import unreal


MAP_PATH = "/Game/Balhwajeom/Maps/Prototype/L_InvestigationPrototype"
TARGET_LABELS = ("Evidence_PigMirror", "Evidence_SnowGlobe")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"Could not load {MAP_PATH}")

actors = {actor.get_actor_label(): actor for actor in actor_subsystem.get_all_level_actors()}
for label in TARGET_LABELS:
    actor = actors.get(label)
    if not actor:
        raise RuntimeError(f"Missing actor: {label}")
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    mesh_asset = mesh.get_editor_property("static_mesh") if mesh else None
    unreal.log(
        f"PROTOTYPE_EVIDENCE_ACTOR Label={label} Class={actor.get_class().get_path_name()} "
        f"ObjectID={actor.get_object_id()} Location={actor.get_actor_location()} "
        f"Rotation={actor.get_actor_rotation()} Scale={actor.get_actor_scale3d()} "
        f"Mesh={mesh_asset.get_path_name() if mesh_asset else 'None'} "
        f"MeshRelativeLocation={mesh.get_editor_property('relative_location') if mesh else 'None'} "
        f"MeshRelativeRotation={mesh.get_editor_property('relative_rotation') if mesh else 'None'} "
        f"MeshRelativeScale={mesh.get_editor_property('relative_scale3d') if mesh else 'None'}"
    )
