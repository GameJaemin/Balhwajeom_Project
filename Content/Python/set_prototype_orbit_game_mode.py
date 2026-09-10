import unreal


MAP_PATH = "/Game/Balhwajeom/Maps/Prototype/L_InvestigationPrototype"
ORBIT_GAME_MODE_PATH = "/Game/Balhwajeom/Blueprints/CameraSystem/BP_OrbitViewGameMode"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"Could not load {MAP_PATH}")

orbit_game_mode_class = unreal.EditorAssetLibrary.load_blueprint_class(ORBIT_GAME_MODE_PATH)
if not orbit_game_mode_class:
    raise RuntimeError(f"Could not load {ORBIT_GAME_MODE_PATH}")

world = unreal.EditorLevelLibrary.get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", orbit_game_mode_class)

if not level_subsystem.save_current_level():
    raise RuntimeError(f"Could not save {MAP_PATH}")

default_object = unreal.get_default_object(orbit_game_mode_class)
default_pawn_class = default_object.get_editor_property("default_pawn_class")
unreal.log(
    f"PROTOTYPE_GAME_MODE_UPDATE Result=Success GameMode={orbit_game_mode_class.get_path_name()} "
    f"DefaultPawn={default_pawn_class.get_path_name() if default_pawn_class else 'None'}"
)
