import unreal


BLUEPRINT_FOLDER = "/Game/Balhwajeom/Blueprints/CameraSystem"
PLAYER_CONTROLLER_NAME = "BP_OrbitViewPlayerController"
PLAYER_CONTROLLER_PATH = f"{BLUEPRINT_FOLDER}/{PLAYER_CONTROLLER_NAME}"
GAME_MODE_PATH = "/Game/Balhwajeom/Blueprints/CameraSystem/BP_OrbitViewGameMode"
HUD_WIDGET_PATH = "/Game/Balhwajeom/UI/HUD/WB_HUID"


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

hud_widget_class = unreal.EditorAssetLibrary.load_blueprint_class(HUD_WIDGET_PATH)
if not hud_widget_class:
    raise RuntimeError(f"Could not load HUD widget class: {HUD_WIDGET_PATH}")

player_controller_blueprint = None
if unreal.EditorAssetLibrary.does_asset_exist(PLAYER_CONTROLLER_PATH):
    player_controller_blueprint = unreal.EditorAssetLibrary.load_asset(PLAYER_CONTROLLER_PATH)
if not player_controller_blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.BalhwajeomCameraPlayerController)
    player_controller_blueprint = asset_tools.create_asset(
        PLAYER_CONTROLLER_NAME,
        BLUEPRINT_FOLDER,
        unreal.Blueprint,
        factory,
    )

if not player_controller_blueprint:
    raise RuntimeError(f"Could not create or load: {PLAYER_CONTROLLER_PATH}")

player_controller_class = player_controller_blueprint.generated_class()
player_controller_defaults = unreal.get_default_object(player_controller_class)
player_controller_defaults.set_editor_property("player_hud_widget_class", hud_widget_class)

unreal.BlueprintEditorLibrary.compile_blueprint(player_controller_blueprint)
if not unreal.EditorAssetLibrary.save_loaded_asset(player_controller_blueprint, False):
    raise RuntimeError(f"Could not save: {PLAYER_CONTROLLER_PATH}")

game_mode_blueprint = unreal.EditorAssetLibrary.load_asset(GAME_MODE_PATH)
if not game_mode_blueprint:
    raise RuntimeError(f"Could not load game mode: {GAME_MODE_PATH}")

game_mode_defaults = unreal.get_default_object(game_mode_blueprint.generated_class())
game_mode_defaults.set_editor_property("player_controller_class", player_controller_class)

unreal.BlueprintEditorLibrary.compile_blueprint(game_mode_blueprint)
if not unreal.EditorAssetLibrary.save_loaded_asset(game_mode_blueprint, False):
    raise RuntimeError(f"Could not save: {GAME_MODE_PATH}")

saved_controller_class = game_mode_defaults.get_editor_property("player_controller_class")
saved_hud_widget_class = player_controller_defaults.get_editor_property(
    "player_hud_widget_class"
)
if saved_controller_class != player_controller_class:
    raise RuntimeError("BP_OrbitViewGameMode did not retain the PlayerController class")
if saved_hud_widget_class != hud_widget_class:
    raise RuntimeError("BP_OrbitViewPlayerController did not retain the HUD widget class")

unreal.log(
    "ORBIT_PLAYER_CONTROLLER_UI Result=Success "
    f"Controller={player_controller_class.get_path_name()} "
    f"HUDWidget={hud_widget_class.get_path_name()} "
    f"GameMode={game_mode_blueprint.generated_class().get_path_name()}"
)
