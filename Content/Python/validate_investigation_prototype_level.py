import unreal


MAP_PATH = "/Game/Balhwajeom/Maps/Prototype/L_InvestigationPrototype"
EXPECTED_GAME_MODE_PATH = "/Game/Balhwajeom/Blueprints/CameraSystem/BP_OrbitViewGameMode.BP_OrbitViewGameMode_C"
EXPECTED_PAWN_PATH = "/Game/Balhwajeom/Blueprints/CameraSystem/BP_OrbitViewCharacter.BP_OrbitViewCharacter_C"
EXPECTED_EVIDENCE = {
    "Evidence_PigMirror": {
        "asset": "/Game/Balhwajeom/Blueprints/Investigation/Evidence/BP_Evidence_PigMirror",
        "class": "/Game/Balhwajeom/Blueprints/Investigation/Evidence/BP_Evidence_PigMirror.BP_Evidence_PigMirror_C",
        "object_id": "OBJ_PIG_MIRROR",
        "location": (680.0, 0.0, 165.0),
        "scale": (0.25, 1.15, 1.15),
    },
    "Evidence_SnowGlobe": {
        "asset": "/Game/Balhwajeom/Blueprints/Investigation/Evidence/BP_Evidence_SnowGlobe",
        "class": "/Game/Balhwajeom/Blueprints/Investigation/Evidence/BP_Evidence_SnowGlobe.BP_Evidence_SnowGlobe_C",
        "object_id": "OBJ_SNOW_GLOBE",
        "location": (600.0, -360.0, 135.0),
        "scale": (0.75, 0.75, 0.75),
    },
}
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    raise RuntimeError(f"Missing prototype map: {MAP_PATH}")
if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"Could not load prototype map: {MAP_PATH}")

actors = actor_subsystem.get_all_level_actors()
labels = {actor.get_actor_label(): actor for actor in actors}
required = {
    "Prototype_PlayerStart",
    "Prototype_Floor",
    "Evidence_PigMirror",
    "Evidence_SnowGlobe",
    "Prototype_KeyLight",
    "Prototype_SkyLight",
    "Prototype_Instructions",
}
missing = sorted(required.difference(labels))
if missing:
    raise RuntimeError(f"Prototype map is missing actors: {missing}")

def tuple_nearly_equal(actual, expected, tolerance=0.01):
    return all(abs(a - e) <= tolerance for a, e in zip(actual, expected))


for label, expected in EXPECTED_EVIDENCE.items():
    if not unreal.EditorAssetLibrary.does_asset_exist(expected["asset"]):
        raise RuntimeError(f"Missing evidence Blueprint: {expected['asset']}")

    actor = labels[label]
    class_path = actor.get_class().get_path_name()
    if class_path != expected["class"]:
        raise RuntimeError(f"{label} has wrong class: {class_path}")
    if str(actor.get_object_id()) != expected["object_id"]:
        raise RuntimeError(f"{label} has wrong ObjectID: {actor.get_object_id()}")

    location = actor.get_actor_location()
    location_tuple = (location.x, location.y, location.z)
    if not tuple_nearly_equal(location_tuple, expected["location"]):
        raise RuntimeError(f"{label} moved unexpectedly: {location_tuple}")

    scale = actor.get_actor_scale3d()
    scale_tuple = (scale.x, scale.y, scale.z)
    if not tuple_nearly_equal(scale_tuple, expected["scale"]):
        raise RuntimeError(f"{label} scale changed unexpectedly: {scale_tuple}")

world = unreal.EditorLevelLibrary.get_editor_world()
game_mode = world.get_world_settings().get_editor_property("default_game_mode")
game_mode_path = game_mode.get_path_name() if game_mode else ""
if game_mode_path != EXPECTED_GAME_MODE_PATH:
    raise RuntimeError(f"Wrong GameMode: {game_mode_path or game_mode}")

game_mode_default = unreal.get_default_object(game_mode)
default_pawn = game_mode_default.get_editor_property("default_pawn_class")
default_pawn_path = default_pawn.get_path_name() if default_pawn else ""
if default_pawn_path != EXPECTED_PAWN_PATH:
    raise RuntimeError(f"Wrong default pawn: {default_pawn_path or default_pawn}")

unreal.log(
    f"INVESTIGATION_PROTOTYPE_LEVEL_VALIDATION Result=Success "
    f"Actors={len(actors)} Evidence=2 Map={MAP_PATH}"
)
