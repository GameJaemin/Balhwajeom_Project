"""Gate the room3 tutorial exit on completing the family photo sentence.

Run through UnrealEditor-Cmd with ``-ExecutePythonScript``.  The script is
idempotent so designers can re-run it after the tutorial flow or room3 map is
merged from another branch.
"""

import unreal


FLOW_PATH = "/Game/Balhwajeom/Data/Tutorial/DA_TutorialFlow_Room2"
MAP_PATH = "/Game/Levels/room3"
EXIT_DOOR_LABEL = "GateDoor_Exit"

ANALYSIS_STEP_ID = "CompleteFamilyPhoto"
ANALYSIS_STAGE_TAG = "Tutorial.Stage.CompleteFamilyPhoto"
TABLET_LOCK_TAG = "Runtime.Lock.Tablet"
FAMILY_PHOTO_SOLVED_TAG = "Evidence.SentenceSolved.PHOTO_01_003"

LOCKED_FEEDBACK_STAGES = (
    (
        (
            "Evidence.State.STATE_01_001_CLEAR",
            "Evidence.State.STATE_01_002_CLEAR",
            "Evidence.State.STATE_01_003_CLEAR",
        ),
        "[F]를 눌러 아직 조사하지 않은 액자를 살펴보자.",
    ),
    (
        (
            "Evidence.Photographed.OBJ_01_001",
            "Evidence.Photographed.OBJ_01_002",
            "Evidence.Photographed.OBJ_01_003",
        ),
        "우클릭으로 카메라를 켜고, 아직 찍지 않은 액자를 촬영해 보자.",
    ),
    (
        (FAMILY_PHOTO_SOLVED_TAG,),
        "[TAB]으로 태블릿을 열고, 여동생 폴더의 가족 사진 추리를 완성해 보자.",
    ),
)


def gameplay_tag(name):
    tag = unreal.GameplayTag()
    if not tag.import_text(name):
        raise RuntimeError(f"Could not resolve gameplay tag {name}")
    return tag


def tag_container(*names):
    container = unreal.GameplayTagContainer()
    if names:
        import_text = f"(GameplayTags=({','.join(names)}))"
        if not container.import_text(import_text):
            raise RuntimeError(f"Could not build gameplay tag container {names}")
    return container


def configure_flow():
    flow = unreal.load_asset(FLOW_PATH)
    if flow is None:
        raise RuntimeError(f"Could not load {FLOW_PATH}")

    steps = list(flow.get_editor_property("steps"))
    analysis_step = None
    done_step = None
    for step in steps:
        step_id = str(step.get_editor_property("step_id"))
        if step_id in ("Talk", ANALYSIS_STEP_ID):
            if analysis_step is not None:
                raise RuntimeError("Tutorial flow contains duplicate analysis steps")
            analysis_step = step
        elif step_id == "Done":
            done_step = step

    if analysis_step is None:
        raise RuntimeError("Tutorial flow has neither Talk nor CompleteFamilyPhoto step")
    if done_step is None:
        raise RuntimeError("Tutorial flow has no Done step")

    analysis_step.set_editor_property("step_id", unreal.Name(ANALYSIS_STEP_ID))
    analysis_step.set_editor_property(
        "stage_tag", gameplay_tag(ANALYSIS_STAGE_TAG)
    )
    analysis_step.set_editor_property("grant_on_enter", tag_container())
    analysis_step.set_editor_property(
        "remove_on_enter", tag_container(TABLET_LOCK_TAG)
    )
    analysis_step.set_editor_property(
        "complete_when_all_tags", tag_container(FAMILY_PHOTO_SOLVED_TAG)
    )
    analysis_step.set_editor_property("complete_when_any_tags", tag_container())
    analysis_step.set_editor_property(
        "dim_mode", unreal.BalhwajeomTutorialDimMode.ALWAYS
    )
    analysis_step.set_editor_property(
        "hint_target", unreal.BalhwajeomTutorialHintTarget.TABLET_ICON
    )

    # The tablet must open while the analysis step is active, not after it finishes.
    done_step.set_editor_property("remove_on_enter", tag_container())

    flow.set_editor_property("steps", steps)
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        flow, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Could not save {FLOW_PATH}")


def configure_room3_exit():
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    if world is None:
        raise RuntimeError(f"Could not load {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    exit_door = None
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() == EXIT_DOOR_LABEL:
            exit_door = actor
            break

    if exit_door is None:
        raise RuntimeError(f"Could not find {EXIT_DOOR_LABEL} in {MAP_PATH}")
    if not isinstance(exit_door, unreal.BalhwajeomGateDoorActor):
        raise RuntimeError(
            f"{EXIT_DOOR_LABEL} is {exit_door.get_class().get_name()}, not BalhwajeomGateDoorActor"
        )

    door_interaction = exit_door.get_door_interaction()
    if door_interaction is None:
        raise RuntimeError(f"{EXIT_DOOR_LABEL} has no DoorInteractionComponent")

    door_interaction.set_editor_property(
        "unlock_requires_tags", tag_container(FAMILY_PHOTO_SOLVED_TAG)
    )
    door_interaction.set_editor_property("unlock_query", unreal.GameplayTagQuery())

    feedback_stages = []
    for required_tags, message in LOCKED_FEEDBACK_STAGES:
        stage = unreal.GateDoorLockedFeedbackStage()
        stage.set_editor_property(
            "complete_when_all_tags", tag_container(*required_tags)
        )
        stage.set_editor_property("incomplete_message", message)
        feedback_stages.append(stage)
    exit_door.set_editor_property("locked_feedback_stages", feedback_stages)

    if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
        raise RuntimeError(f"Could not save {MAP_PATH}")


def main():
    configure_flow()
    configure_room3_exit()
    unreal.log(
        "Configured room3 tutorial completion and ordered exit-door guidance"
    )


main()
