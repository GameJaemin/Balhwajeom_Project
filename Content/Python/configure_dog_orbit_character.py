import unreal


CHARACTER_PATH = "/Game/Balhwajeom/Blueprints/CameraSystem/BP_OrbitViewCharacter"
LEGACY_PATH = "/Game/Balhwajeom/Blueprints/CameraSystem/BP_OrbitViewCharacter_Legacy"
DOG_MESH_PATH = "/Game/Balhwajeom/Characters/Player/Dog_Human"
IDLE_PATH = "/Game/Balhwajeom/Characters/Player/Dog_Idle_Anim"
WALK_PATH = "/Game/Balhwajeom/Characters/Player/Dog_Walking_Anim"


def require_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"Missing required asset: {path}")
    return asset


blueprint = require_asset(CHARACTER_PATH)
dog_mesh = require_asset(DOG_MESH_PATH)
idle_animation = require_asset(IDLE_PATH)
walk_animation = require_asset(WALK_PATH)

mesh_skeleton = dog_mesh.get_editor_property("skeleton")
if idle_animation.get_editor_property("skeleton") != mesh_skeleton:
    raise RuntimeError("Dog_Idle_Anim does not use Dog_Human's skeleton")
if walk_animation.get_editor_property("skeleton") != mesh_skeleton:
    raise RuntimeError("Dog_Walking_Anim does not use Dog_Human's skeleton")

# A one-time copy keeps the pre-dog Blueprint defaults available as a drop-in pawn.
if not unreal.EditorAssetLibrary.does_asset_exist(LEGACY_PATH):
    if not unreal.EditorAssetLibrary.duplicate_asset(CHARACTER_PATH, LEGACY_PATH):
        raise RuntimeError(f"Could not create legacy backup: {LEGACY_PATH}")
    unreal.log(f"DOG_ORBIT_SETUP CreatedBackup={LEGACY_PATH}")
else:
    unreal.log(f"DOG_ORBIT_SETUP KeptExistingBackup={LEGACY_PATH}")

character_class = unreal.EditorAssetLibrary.load_blueprint_class(CHARACTER_PATH)
if not character_class:
    raise RuntimeError(f"Could not load generated class for {CHARACTER_PATH}")

cdo = unreal.get_default_object(character_class)
cdo.set_editor_property("allow_camera_orbit", True)
cdo.set_editor_property("initial_orbit_pitch", -12.0)
cdo.set_editor_property("walk_speed", 300.0)
cdo.set_editor_property("sprint_speed", 500.0)
cdo.set_editor_property("idle_animation", idle_animation)
cdo.set_editor_property("walk_animation", walk_animation)
cdo.set_editor_property("walk_animation_threshold", 5.0)

mesh = cdo.get_editor_property("mesh")
mesh.set_skeletal_mesh_asset(dog_mesh)
mesh.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -96.0))
mesh.set_editor_property("relative_rotation", unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
mesh.set_editor_property("relative_scale3d", unreal.Vector(0.5, 0.5, 0.5))

camera_boom = cdo.get_editor_property("camera_boom")
camera_boom.set_editor_property("target_arm_length", 500.0)
camera_boom.set_editor_property("socket_offset", unreal.Vector(0.0, 0.0, 70.0))
camera_boom.set_editor_property("do_collision_test", True)
camera_boom.set_editor_property("enable_camera_lag", True)
camera_boom.set_editor_property("camera_lag_speed", 12.0)

camera = cdo.get_editor_property("top_down_camera")
camera.set_editor_property("field_of_view", 75.0)

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save {CHARACTER_PATH}")
unreal.EditorAssetLibrary.save_asset(LEGACY_PATH, only_if_is_dirty=False)

# Reload and verify the persistent defaults used by the game mode.
saved_class = unreal.EditorAssetLibrary.load_blueprint_class(CHARACTER_PATH)
saved_cdo = unreal.get_default_object(saved_class)
saved_mesh = saved_cdo.get_editor_property("mesh").get_editor_property("skeletal_mesh_asset")
saved_idle = saved_cdo.get_editor_property("idle_animation")
saved_walk = saved_cdo.get_editor_property("walk_animation")
if saved_mesh != dog_mesh or saved_idle != idle_animation or saved_walk != walk_animation:
    raise RuntimeError("Saved Blueprint defaults did not retain the dog locomotion assets")

unreal.log(
    "DOG_ORBIT_SETUP Result=Success "
    f"Character={CHARACTER_PATH} Mesh={saved_mesh.get_path_name()} "
    f"Idle={saved_idle.get_path_name()} Walk={saved_walk.get_path_name()} "
    f"Legacy={LEGACY_PATH}"
)
