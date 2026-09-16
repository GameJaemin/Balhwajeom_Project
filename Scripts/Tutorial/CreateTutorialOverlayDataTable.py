"""Create or refresh DT_TutorialOverlay with the approved initial rows.

Run through UnrealEditor-Cmd with ``-ExecutePythonScript`` after compiling
FTutorialOverlayDefinition. RequiredTags and Images intentionally start empty;
empty RequiredTags must not be treated as an immediately satisfied trigger when
the runtime presenter is connected later.
"""

from pathlib import Path
import json

import unreal


ASSET_FOLDER = "/Game/Balhwajeom/Data/Investigation"
ASSET_NAME = "DT_TutorialOverlay"
ASSET_PATH = f"{ASSET_FOLDER}/{ASSET_NAME}"

ROWS = (
    (
        "StatementIntro",
        "진술서를 확인해보세요.\n진술 반증서를 작성하기 위한 단어를 찾아야합니다.",
        "Tutorial.Overlay.Seen.StatementIntro",
    ),
    (
        "MovementAndLook",
        "WASD로 이동하고 마우스로 회전",
        "Tutorial.Overlay.Seen.MovementAndLook",
    ),
    (
        "Interaction",
        "F 상호작용 설명",
        "Tutorial.Overlay.Seen.Interaction",
    ),
    (
        "PhotoCamera",
        "카메라 설명",
        "Tutorial.Overlay.Seen.PhotoCamera",
    ),
    (
        "MemoryObject",
        "회상(빛나는 오브젝트) 설명",
        "Tutorial.Overlay.Seen.MemoryObject",
    ),
    (
        "Tablet",
        "태블릿 설명",
        "Tutorial.Overlay.Seen.Tablet",
    ),
    (
        "SisterFolder",
        "여동생 폴더 설명",
        "Tutorial.Overlay.Seen.SisterFolder",
    ),
    (
        "PhotoSentencePuzzle",
        "단서 사진을 클릭하고 문장을 조합하세요",
        "Tutorial.Overlay.Seen.PhotoSentencePuzzle",
    ),
)


def gameplay_tag(name):
    tag = unreal.GameplayTag()
    if not tag.import_text(name):
        raise RuntimeError(f"Could not resolve gameplay tag {name}")
    return tag


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
    for row_name, overlay_text, completion_tag_name in ROWS:
        # Resolve before import so a misspelled tag fails the generator loudly.
        gameplay_tag(completion_tag_name)
        rows_json.append(
            {
                "Name": row_name,
                "RequiredTags": "(GameplayTags=())",
                "Images": [],
                "OverlayText": overlay_text,
                "CompletionTag": completion_tag_name,
            }
        )

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
