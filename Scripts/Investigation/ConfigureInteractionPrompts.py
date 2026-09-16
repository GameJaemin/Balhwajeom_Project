"""Set state-specific WBP_Interact action text on the live evidence DataTable."""

import csv
import io

import unreal


TABLE_PATH = "/Game/Balhwajeom/Data/Investigation/DT_EvidenceStates"
PROMPTS = {
    "STATE_01_001_DUST": "먼지 털기",
    "STATE_01_001_MEMORY": "회상하기",
    "STATE_01_002_DUST": "먼지 털기",
    "STATE_01_002_MEMORY": "회상하기",
    "STATE_01_003_DUST": "먼지 털기",
    "STATE_01_003_MEMORY": "회상하기",
    "STATE_01_004_CLOSED": "일기장 보기",
    "STATE_01_004_OPEN": "일기장 보기",
    "STATE_01_004_MEMORY": "회상하기",
    "STATE_01_005_FRONT": "뒤집어 보기",
    "STATE_01_014_CLOSED": "문 열기",
    "STATE_01_016_NORMAL": "쪽지 보기",
    "STATE_01_020_NORMAL": "쪽지 보기",
    "STATE_01_024_NORMAL": "쪽지 보기",
    "STATE_01_025_NORMAL": "쪽지 보기",
}


def main():
    table = unreal.load_asset(TABLE_PATH)
    if table is None:
        raise RuntimeError(f"Could not load {TABLE_PATH}")

    csv_text = unreal.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
    reader = csv.DictReader(io.StringIO(csv_text))
    field_names = reader.fieldnames
    if not field_names or "InteractionPromptText" not in field_names:
        raise RuntimeError("DT_EvidenceStates has no InteractionPromptText column")

    rows = list(reader)
    row_name_field = field_names[0]
    patched = set()
    for row in rows:
        row_name = row.get(row_name_field, "")
        if row_name in PROMPTS:
            row["InteractionPromptText"] = PROMPTS[row_name]
            patched.add(row_name)

    missing = set(PROMPTS) - patched
    if missing:
        unreal.log_warning(
            f"Skipped prompt rows that are not in the live DataTable: {sorted(missing)}"
        )

    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=field_names, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    if unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(
        table, output.getvalue()
    ) is False:
        raise RuntimeError("Failed to update DT_EvidenceStates")
    if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {TABLE_PATH}")

    unreal.log(f"Configured {len(patched)} interaction prompts in {TABLE_PATH}")


main()
