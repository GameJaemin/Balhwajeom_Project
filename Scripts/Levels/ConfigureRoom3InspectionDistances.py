"""Retune the per-object inspection label distances of the room3 evidence actors.

Run through UnrealEditor-Cmd with ``-ExecutePythonScript``.  The script is
idempotent so designers can re-run it after room3 is merged from another branch.

Each placed ABalhwajeomEvidenceActor owns an InspectionComponent whose three
thresholds drive the distance labels: CloseDistance shows NearLabel,
MiddleDistance shows MidLabel, and MaxDisplayDistance is the far cutoff.
"""

import unreal


MAP_PATH = "/Game/Levels/room3"

# (CloseDistance, MiddleDistance, MaxDisplayDistance) for the objects that need
# thresholds other than DEFAULT_DISTANCES.
DISTANCE_GROUPS = (
    (("OBJ_01_001", "OBJ_01_002"), (200.0, 400.0, 600.0)),
    (
        (
            "OBJ_01_003",
            "OBJ_01_017",
            "OBJ_01_018",
            "OBJ_01_023",
            "OBJ_01_024",
            # Phase-gating ceiling frames share the same band as the objects above.
            "Obstacle_Phase01",
            "Obstacle_Phase02",
        ),
        (180.0, 350.0, 420.0),
    ),
    (("OBJ_01_019", "OBJ_01_021"), (200.0, 280.0, 360.0)),
)

# Applied to every other evidence actor in the level.
DEFAULT_DISTANCES = (150.0, 240.0, 310.0)


def build_distance_table():
    table = {}
    for object_ids, distances in DISTANCE_GROUPS:
        for object_id in object_ids:
            if object_id in table:
                raise RuntimeError(f"{object_id} is listed in two distance groups")
            table[object_id] = distances
    return table


def main():
    distances_by_object_id = build_distance_table()

    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    if world is None:
        raise RuntimeError(f"Could not load {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    evidence_actors = [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if isinstance(actor, unreal.BalhwajeomEvidenceActor)
    ]
    if not evidence_actors:
        raise RuntimeError(f"{MAP_PATH} contains no BalhwajeomEvidenceActor")

    configured = []
    unidentified = []
    for actor in sorted(evidence_actors, key=lambda placed: placed.get_actor_label()):
        label = actor.get_actor_label()
        object_id = str(actor.get_object_id())
        if object_id in ("", "None"):
            unidentified.append(label)
            continue

        inspection = actor.get_inspection_component()
        if inspection is None:
            raise RuntimeError(f"{label} has no InspectionComponent")

        close, middle, maximum = distances_by_object_id.get(
            object_id, DEFAULT_DISTANCES
        )
        inspection.set_editor_property("close_distance", close)
        inspection.set_editor_property("middle_distance", middle)
        inspection.set_editor_property("max_display_distance", maximum)
        configured.append((object_id, label, close, middle, maximum))

    for object_id, label, close, middle, maximum in sorted(configured):
        unreal.log(
            f"{object_id} ({label}): {close:.0f} / {middle:.0f} / {maximum:.0f}"
        )
    for label in unidentified:
        unreal.log_warning(f"{label} has no ObjectID, so its distances were left alone")

    missing = sorted(
        set(distances_by_object_id) - {entry[0] for entry in configured}
    )
    if missing:
        raise RuntimeError(f"{MAP_PATH} has no evidence actor for {', '.join(missing)}")

    if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
        raise RuntimeError(f"Could not save {MAP_PATH}")

    unreal.log(
        f"Retuned inspection distances for {len(configured)} room3 evidence objects"
    )


main()
