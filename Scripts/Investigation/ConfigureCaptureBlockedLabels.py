"""Configure state-specific camera guidance on the live evidence DataTable.

The table is exported first so fields omitted from the authored CSV and local
changes on unrelated rows are preserved. Run through UnrealEditor-Cmd with
-ExecutePythonScript after compiling FEvidenceStateDefinition.
"""

import csv
import io

import unreal


TABLE_PATH = "/Game/Balhwajeom/Data/Investigation/DT_EvidenceStates"
# The sub label only appears on a focused state that cannot be captured, so every
# row below is authored guidance for one of two situations: the state opens up to a
# photo after an F interaction, or it is never a photo target at all. A row left out
# of this table falls back to the shared line in BalhwajeomEvidenceFocusGuideLayout.
DUSTY_PHOTO = "먼지 때문에 사진이 제대로 보이지 않는다."
BEFORE_CAPTURE = "촬영하기 전에 먼저 살펴볼 필요가 있을 것 같다."
NEVER_CAPTURABLE = "이건 굳이 사진으로 남기지 않아도 될 것 같다."

LABELS = {
    # Tutorial frames: the photo itself is unreadable until the dust is off.
    "STATE_01_001_DUST": DUSTY_PHOTO,
    "STATE_01_002_DUST": DUSTY_PHOTO,
    "STATE_01_003_DUST": DUSTY_PHOTO,
    # One F interaction opens the capturable state on each of these.
    "STATE_01_004_CLOSED": BEFORE_CAPTURE,
    "STATE_01_005_FRONT": BEFORE_CAPTURE,
    "STATE_01_016_NORMAL": BEFORE_CAPTURE,
    "STATE_01_020_NORMAL": BEFORE_CAPTURE,
    "STATE_01_024_NORMAL": BEFORE_CAPTURE,
    "STATE_01_025_NORMAL": BEFORE_CAPTURE,
    # Progression obstacles, which never become photo targets.
    "STATE_Obstacle_Phase01_NORMAL": NEVER_CAPTURABLE,
    "STATE_Obstacle_Phase02_NORMAL": NEVER_CAPTURABLE,
}


def main():
    table = unreal.load_asset(TABLE_PATH)
    if table is None:
        raise RuntimeError(f"Could not load {TABLE_PATH}")

    csv_text = unreal.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
    reader = csv.DictReader(io.StringIO(csv_text))
    field_names = reader.fieldnames
    if not field_names or "CaptureBlockedLabel" not in field_names:
        raise RuntimeError("DT_EvidenceStates has no CaptureBlockedLabel column")

    rows = list(reader)
    row_name_field = field_names[0]
    patched = set()
    for row in rows:
        row_name = row.get(row_name_field, "")
        label = LABELS.get(row_name)
        if label is not None:
            row["CaptureBlockedLabel"] = label
            patched.add(row_name)

    missing = set(LABELS) - patched
    if missing:
        raise RuntimeError(f"Missing capture guidance rows: {sorted(missing)}")

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

    unreal.log(f"Configured {len(patched)} capture-blocked labels in {TABLE_PATH}")


main()
