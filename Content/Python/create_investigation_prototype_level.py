import unreal


MAP_PATH = "/Game/Balhwajeom/Maps/Prototype/L_InvestigationPrototype"
ORBIT_GAME_MODE_PATH = "/Game/Balhwajeom/Blueprints/CameraSystem/BP_OrbitViewGameMode"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    if not level_subsystem.load_level(MAP_PATH):
        raise RuntimeError(f"Could not load {MAP_PATH}")
else:
    if not level_subsystem.new_level(MAP_PATH):
        raise RuntimeError(f"Could not create {MAP_PATH}")

world = unreal.EditorLevelLibrary.get_editor_world()
world_settings = world.get_world_settings()
orbit_game_mode_class = unreal.EditorAssetLibrary.load_blueprint_class(ORBIT_GAME_MODE_PATH)
if not orbit_game_mode_class:
    raise RuntimeError(f"Could not load {ORBIT_GAME_MODE_PATH}")
world_settings.set_editor_property("default_game_mode", orbit_game_mode_class)

# This generator owns the prototype map, so rebuilding it intentionally clears its actors.
for actor in actor_subsystem.get_all_level_actors():
    if not isinstance(actor, unreal.WorldSettings):
        actor_subsystem.destroy_actor(actor)


def spawn_static(label, mesh_path, location, scale, rotation=(0.0, 0.0, 0.0)):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(*location),
        unreal.Rotator(rotation[1], rotation[2], rotation[0]),
    )
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    component = actor.static_mesh_component
    component.set_static_mesh(unreal.load_asset(mesh_path))
    component.set_collision_profile_name("BlockAll")
    return actor


spawn_static("Prototype_Floor", "/Engine/BasicShapes/Cube", (450, 0, -30), (12, 8, 0.3))
spawn_static("Back_Wall", "/Engine/BasicShapes/Cube", (1050, 0, 220), (0.25, 8, 2.5))
spawn_static("North_Wall", "/Engine/BasicShapes/Cube", (450, 780, 220), (12, 0.25, 2.5))
spawn_static("South_Wall", "/Engine/BasicShapes/Cube", (450, -780, 220), (12, 0.25, 2.5))
spawn_static("Mirror_Pedestal", "/Engine/BasicShapes/Cube", (680, 0, 45), (1.3, 1.3, 0.45))
spawn_static("SnowGlobe_Pedestal", "/Engine/BasicShapes/Cube", (600, -360, 45), (1.0, 1.0, 0.45))

player_start = actor_subsystem.spawn_actor_from_class(
    unreal.PlayerStart, unreal.Vector(-50, 0, 100), unreal.Rotator(0, 0, 0)
)
player_start.set_actor_label("Prototype_PlayerStart")

mirror = actor_subsystem.spawn_actor_from_class(
    unreal.BalhwajeomEvidenceActor, unreal.Vector(680, 0, 165), unreal.Rotator(0, 0, 0)
)
mirror.set_actor_label("Evidence_PigMirror")
mirror.configure_investigation_object("OBJ_PIG_MIRROR")
mirror_mesh = mirror.get_component_by_class(unreal.StaticMeshComponent)
mirror_mesh.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
mirror_mesh.set_world_scale3d(unreal.Vector(0.25, 1.15, 1.15))

globe = actor_subsystem.spawn_actor_from_class(
    unreal.BalhwajeomEvidenceActor, unreal.Vector(600, -360, 135), unreal.Rotator(0, 0, 0)
)
globe.set_actor_label("Evidence_SnowGlobe")
globe.configure_investigation_object("OBJ_SNOW_GLOBE")
globe_mesh = globe.get_component_by_class(unreal.StaticMeshComponent)
globe_mesh.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Sphere"))
globe_mesh.set_world_scale3d(unreal.Vector(0.75, 0.75, 0.75))

directional = actor_subsystem.spawn_actor_from_class(
    unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-35, -35, 0)
)
directional.set_actor_label("Prototype_KeyLight")
directional.light_component.set_editor_property("intensity", 4.0)

sky = actor_subsystem.spawn_actor_from_class(
    unreal.SkyLight, unreal.Vector(0, 0, 400), unreal.Rotator()
)
sky.set_actor_label("Prototype_SkyLight")
sky.light_component.set_editor_property("intensity", 1.2)

for index, position in enumerate(((450, 300, 400), (650, -300, 350))):
    light = actor_subsystem.spawn_actor_from_class(
        unreal.PointLight, unreal.Vector(*position), unreal.Rotator()
    )
    light.set_actor_label(f"Prototype_FillLight_{index + 1}")
    light.light_component.set_editor_property("intensity", 3500.0)
    light.light_component.set_editor_property("attenuation_radius", 900.0)

instructions = actor_subsystem.spawn_actor_from_class(
    unreal.TextRenderActor, unreal.Vector(250, 0, 320), unreal.Rotator(0, 180, 0)
)
instructions.set_actor_label("Prototype_Instructions")
text = instructions.text_render
text.set_editor_property(
    "text",
    "FINAL PROTOTYPE\nWASD 이동 / F 조사\nRMB 카메라 / LMB 촬영\nTAB 태블릿",
)
text.set_editor_property("world_size", 34.0)
text.set_editor_property("text_render_color", unreal.Color(255, 210, 120, 255))

if not level_subsystem.save_current_level():
    raise RuntimeError(f"Could not save {MAP_PATH}")

unreal.log(f"INVESTIGATION_PROTOTYPE_LEVEL Result=Success Map={MAP_PATH}")
