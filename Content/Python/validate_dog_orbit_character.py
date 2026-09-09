import unreal


MAP_PATH = "/Game/Balhwajeom/Maps/Prototype/L_InvestigationPrototype"
CHARACTER_PATH = "/Game/Balhwajeom/Blueprints/CameraSystem/BP_OrbitViewCharacter"
DOG_MESH_PATH = "/Game/Balhwajeom/Characters/Player/Dog_Human"
IDLE_PATH = "/Game/Balhwajeom/Characters/Player/Dog_Idle_Anim"
WALK_PATH = "/Game/Balhwajeom/Characters/Player/Dog_Walking_Anim"
LEGACY_PATH = "/Game/Balhwajeom/Blueprints/CameraSystem/BP_OrbitViewCharacter_Legacy"


level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"Could not load {MAP_PATH}")

character_class = unreal.EditorAssetLibrary.load_blueprint_class(CHARACTER_PATH)
dog_mesh = unreal.EditorAssetLibrary.load_asset(DOG_MESH_PATH)
idle = unreal.EditorAssetLibrary.load_asset(IDLE_PATH)
walk = unreal.EditorAssetLibrary.load_asset(WALK_PATH)
if not all((character_class, dog_mesh, idle, walk)):
    raise RuntimeError("Dog orbit character assets are incomplete")
if not unreal.EditorAssetLibrary.does_asset_exist(LEGACY_PATH):
    raise RuntimeError("Legacy orbit character backup is missing")

actor = actor_subsystem.spawn_actor_from_class(
    character_class, unreal.Vector(0.0, 0.0, 1000.0), unreal.Rotator()
)
if not actor:
    raise RuntimeError("Could not spawn BP_OrbitViewCharacter")

try:
    mesh_component = actor.get_editor_property("mesh")
    camera_boom = actor.get_editor_property("camera_boom")
    if mesh_component.get_editor_property("skeletal_mesh_asset") != dog_mesh:
        raise RuntimeError("Spawned character does not use Dog_Human")
    if actor.get_editor_property("idle_animation") != idle:
        raise RuntimeError("Spawned character does not use Dog_Idle_Anim")
    if actor.get_editor_property("walk_animation") != walk:
        raise RuntimeError("Spawned character does not use Dog_Walking_Anim")
    if not actor.get_editor_property("allow_camera_orbit"):
        raise RuntimeError("Spawned character does not have orbit enabled")
    if abs(camera_boom.get_editor_property("target_arm_length") - 500.0) > 0.01:
        raise RuntimeError("Spawned character does not have the third-person camera distance")

    unreal.log(
        "DOG_ORBIT_VALIDATION Result=Success "
        f"Class={actor.get_class().get_path_name()} Mesh={dog_mesh.get_path_name()} "
        f"Idle={idle.get_path_name()} Walk={walk.get_path_name()} "
        f"CameraArm={camera_boom.get_editor_property('target_arm_length')}"
    )
finally:
    actor_subsystem.destroy_actor(actor)
