"""Hand the tutorial's explanation duty to the overlays and leave the hints as pointers.

Run through UnrealEditor-Cmd with ``-ExecutePythonScript``. Idempotent, so it can be
re-run after DA_TutorialFlow_Room2 is merged from another branch.

Two things change for every step.

* The dim is switched off everywhere. The overlays already darken the screen while they
  are up, and a second, permanent dim underneath them made the room read as unlit.
* Each highlight waits for its own ``Tutorial.Overlay.Seen.*`` tag, so the icon starts
  blinking only after the player has read the screen that explains it.

The TabletIntro step keeps its timing role but loses its presentation entirely: it fired
the moment the game opened, before the player had been told anything.
"""

import unreal


FLOW_PATH = "/Game/Balhwajeom/Data/Tutorial/DA_TutorialFlow_Room2"

# StepID -> (HintTarget, HintRequiredTags). A step missing from here is left with no
# presentation at all. Index 0 has no StepID, so it is addressed by position.
NO_HINT = (unreal.BalhwajeomTutorialHintTarget.NONE, ())

STEP_PRESENTATION = {
    "": NO_HINT,
    # Was a blinking tablet icon over a dimmed screen the instant the game opened.
    "TabletIntro": NO_HINT,
    "DustTeach": (
        unreal.BalhwajeomTutorialHintTarget.INTERACT_PROMPT,
        ("Tutorial.Overlay.Seen.Interaction",),
    ),
    "PhotoPrompt": (
        unreal.BalhwajeomTutorialHintTarget.PHOTO_CAMERA_ICON,
        ("Tutorial.Overlay.Seen.PhotoCamera",),
    ),
    # The [F] 회상하기 prompt on a photographed frame, once the memory screen is read.
    "Photograph": (
        unreal.BalhwajeomTutorialHintTarget.INTERACT_PROMPT,
        ("Tutorial.Overlay.Seen.MemoryObject",),
    ),
    "CompleteFamilyPhoto": (
        unreal.BalhwajeomTutorialHintTarget.TABLET_ICON,
        ("Tutorial.Overlay.Seen.Tablet",),
    ),
    "Done": NO_HINT,
}


def gameplay_tag(name):
    tag = unreal.GameplayTag()
    if not tag.import_text(name):
        raise RuntimeError(f"Could not resolve gameplay tag {name}")
    return tag


def tag_container(names):
    container = unreal.GameplayTagContainer()
    if not names:
        return container
    joined = ",".join(f'(TagName="{name}")' for name in names)
    if not container.import_text(f"(GameplayTags=({joined}))"):
        raise RuntimeError(f"Could not build a tag container from {names}")
    return container


def main():
    flow = unreal.load_asset(FLOW_PATH)
    if flow is None:
        raise RuntimeError(f"Could not load {FLOW_PATH}")

    steps = list(flow.get_editor_property("steps"))
    for index, step in enumerate(steps):
        step_id = str(step.get_editor_property("step_id"))
        if step_id == "None":
            step_id = ""
        if step_id not in STEP_PRESENTATION:
            raise RuntimeError(
                f"Step {index} ('{step_id}') is not covered by STEP_PRESENTATION"
            )

        hint_target, required_tag_names = STEP_PRESENTATION[step_id]
        for name in required_tag_names:
            # Resolve first so a misspelled tag fails here rather than silently gating
            # the highlight off forever.
            gameplay_tag(name)

        step.set_editor_property("dim_mode", unreal.BalhwajeomTutorialDimMode.OFF)
        step.set_editor_property("hint_target", hint_target)
        step.set_editor_property("hint_required_tags", tag_container(required_tag_names))
        steps[index] = step
        unreal.log(
            f"TUTORIAL_HINT_GATE step={index} id={step_id or '(unnamed)'} "
            f"hint={hint_target} required={','.join(required_tag_names) or '(none)'}"
        )

    flow.set_editor_property("steps", steps)
    if not unreal.EditorAssetLibrary.save_loaded_asset(flow, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {FLOW_PATH}")
    unreal.log(f"TUTORIAL_HINT_GATE Result=Success Steps={len(steps)}")


main()
