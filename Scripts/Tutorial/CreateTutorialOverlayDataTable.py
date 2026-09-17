"""Create or refresh DT_TutorialOverlay with the approved initial rows.

Run through UnrealEditor-Cmd with ``-ExecutePythonScript`` after compiling
FTutorialOverlayDefinition. RequiredTags and Images intentionally start empty;
empty RequiredTags must not be treated as an immediately satisfied trigger when
the runtime presenter is connected later.

Row Name and OverlayID are kept identical, the same rule the other investigation
tables follow. OverlayTitle and OverlayText are placeholder copy until the final
tutorial wording is written.

Images are named by asset, not by file path: every overlay texture lives in
IMAGE_FOLDER and is put there by ImportOverlayImage.py, so regenerating this table
never needs the source images.

RequiredTags follow one rule: the previous row's Tutorial.Overlay.Seen tag plus this
row's own moment. OVL_01_002 waits for the statement tablet the intro opens to be
closed again, so the movement screen arrives on the world rather than on the tablet. Chaining on the previous row keeps the tutorial in order even when a
later moment happens first -- photographing all three frames without leaving camera
mode would otherwise show the tablet screen before the memory screen.
"""

from pathlib import Path
import json

import unreal


ASSET_FOLDER = "/Game/Balhwajeom/Data/Investigation"
ASSET_NAME = "DT_TutorialOverlay"
ASSET_PATH = f"{ASSET_FOLDER}/{ASSET_NAME}"
IMAGE_FOLDER = "/Game/Balhwajeom/UI/Tutorial/Overlay"

ROWS = (
    (
        "OVL_01_001",
        ("Tutorial.Trigger.GameplayStarted",),
        ("T_OVL_01_001_01",),
        "진술서",
        "진술서를 확인해보세요.\n진술 반증서를 작성하기 위한 단어를 찾아야합니다.",
        "Tutorial.Overlay.Seen.StatementIntro",
    ),
    (
        "OVL_01_002",
        (
            "Tutorial.Overlay.Seen.StatementIntro",
            "Tutorial.Trigger.TabletClosed",
        ),
        (),
        "이동과 시점",
        "WASD로 이동하고 마우스로 회전",
        "Tutorial.Overlay.Seen.MovementAndLook",
    ),
    (
        "OVL_01_003",
        (
            "Tutorial.Overlay.Seen.MovementAndLook",
            "Tutorial.Trigger.InteractPromptShown",
        ),
        (),
        "상호작용",
        "F 상호작용 설명",
        "Tutorial.Overlay.Seen.Interaction",
    ),
    (
        "OVL_01_004",
        (
            "Tutorial.Overlay.Seen.Interaction",
            "Tutorial.Trigger.InteractCompleted",
        ),
        (),
        "카메라",
        "카메라 설명",
        "Tutorial.Overlay.Seen.PhotoCamera",
    ),
    (
        "OVL_01_005",
        (
            "Tutorial.Overlay.Seen.PhotoCamera",
            "Tutorial.Trigger.PhotoCaptureCompleted",
        ),
        (),
        "회상",
        "회상(빛나는 오브젝트) 설명",
        "Tutorial.Overlay.Seen.MemoryObject",
    ),
    (
        "OVL_01_006",
        (
            "Tutorial.Overlay.Seen.MemoryObject",
            "Evidence.Photographed.OBJ_01_001",
            "Evidence.Photographed.OBJ_01_002",
            "Evidence.Photographed.OBJ_01_003",
        ),
        (),
        "태블릿",
        "태블릿 설명",
        "Tutorial.Overlay.Seen.Tablet",
    ),
    (
        "OVL_01_007",
        (
            "Tutorial.Overlay.Seen.Tablet",
            "Tutorial.Trigger.TabletOpened",
        ),
        (),
        "여동생 폴더",
        "여동생 폴더 설명",
        "Tutorial.Overlay.Seen.SisterFolder",
    ),
    (
        "OVL_01_008",
        (
            "Tutorial.Overlay.Seen.SisterFolder",
            "Tutorial.Trigger.SisterFolderOpened",
        ),
        (),
        "사진 추리",
        "단서 사진을 클릭하고 문장을 조합하세요",
        "Tutorial.Overlay.Seen.PhotoSentencePuzzle",
    ),
)


def gameplay_tag(name):
    tag = unreal.GameplayTag()
    if not tag.import_text(name):
        raise RuntimeError(f"Could not resolve gameplay tag {name}")
    return tag


def required_tags_text(tag_names):
    """FGameplayTagContainer import text. An empty container never satisfies a row."""
    joined = ",".join(f'(TagName="{name}")' for name in tag_names)
    return f"(GameplayTags=({joined}))"


def image_object_path(asset_name):
    """Fails here rather than leaving a silently broken soft reference in the table."""
    object_path = f"{IMAGE_FOLDER}/{asset_name}.{asset_name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(object_path):
        raise RuntimeError(
            f"{object_path} is missing. Import it with ImportOverlayImage.py first."
        )
    return object_path


def create_or_load_table():
    row_struct = unreal.TutorialOverlayDefinition.static_struct()
    table = unreal.load_asset(ASSET_PATH)
    if table is None:
        factory = unreal.DataTableFactory()
        factory.set_editor_property("struct", row_struct)
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME,
            ASSET_FOLDER,
            unreal.DataTable,
            factory,
        )

    if table is None:
        raise RuntimeError(f"Could not create {ASSET_PATH}")

    actual_struct = unreal.DataTableFunctionLibrary.get_data_table_row_struct(table)
    if actual_struct != row_struct:
        raise RuntimeError(f"{ASSET_PATH} uses the wrong row struct")
    return table


def populate_table(table):
    row_struct = unreal.TutorialOverlayDefinition.static_struct()
    rows_json = []
    for (
        row_name,
        required_tag_names,
        image_names,
        overlay_title,
        overlay_text,
        completion_tag_name,
    ) in ROWS:
        # Resolve before import so a misspelled tag fails the generator loudly.
        gameplay_tag(completion_tag_name)
        for required_tag_name in required_tag_names:
            gameplay_tag(required_tag_name)
        rows_json.append(
            {
                "Name": row_name,
                "OverlayID": row_name,
                "RequiredTags": required_tags_text(required_tag_names),
                "Images": [image_object_path(name) for name in image_names],
                "OverlayTitle": overlay_title,
                "OverlayText": overlay_text,
                "CompletionTag": completion_tag_name,
            }
        )

    # The JSON importer empties the table first, so renamed rows do not linger.
    payload = json.dumps(rows_json, ensure_ascii=False)
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(
        table,
        payload,
        row_struct,
    ):
        raise RuntimeError(f"Could not populate {ASSET_PATH}")


def export_csv(table):
    project_dir = Path(unreal.Paths.project_dir())
    csv_path = project_dir / "Scripts" / "Investigation" / "DT_TutorialOverlay.csv"
    if not unreal.DataTableFunctionLibrary.export_data_table_to_csv_file(
        table,
        str(csv_path),
    ):
        raise RuntimeError(f"Could not export {csv_path}")


def main():
    table = create_or_load_table()
    populate_table(table)
    if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {ASSET_PATH}")
    export_csv(table)
    unreal.log(f"TUTORIAL_OVERLAY_TABLE Result=Success Rows={len(ROWS)}")


main()
