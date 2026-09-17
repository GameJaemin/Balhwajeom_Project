"""Configure state-specific camera guidance on the live evidence DataTable.

The table is exported first so fields omitted from the authored CSV and local
changes on unrelated rows are preserved. Run through UnrealEditor-Cmd with
-ExecutePythonScript after compiling FEvidenceStateDefinition.
"""

import csv
import io

import unreal


TABLE_PATH = "/Game/Balhwajeom/Data/Investigation/DT_EvidenceStates"
LABELS = {
    "STATE_01_001_DUST": "사진을 찍기 전에 먼지부터 털어 보자.",
    "STATE_01_002_DUST": "사진을 찍기 전에 먼지부터 털어 보자.",
    "STATE_01_003_DUST": "사진을 찍기 전에 먼지부터 털어 보자.",
    "STATE_01_004_CLOSED": "일기장을 먼저 펼쳐 보자.",
    "STATE_01_005_FRONT": "고데기를 뒤집어 반대편을 확인해 보자.",
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
