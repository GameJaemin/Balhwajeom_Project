"""Make every diary state open the diary modal, then play 3D text after it closes.

The live table is exported and patched in place so unrelated editor-authored fields are
preserved. Run through UnrealEditor-Cmd with -ExecutePythonScript.
"""

import csv
import io

import unreal


TABLE_PATH = "/Game/Balhwajeom/Data/Investigation/DT_EvidenceStates"
DIARY_OBJECT_ID = "OBJ_01_004"
DIARY_PHOTO_ID = "PHOTO_01_004"
DIARY_WIDGET_CLASS = (
    "WidgetBlueprintGeneratedClass'/Game/Balhwajeom/UI/Diary/"
    "WBP_Diary.WBP_Diary_C'"
)


def main():
    table = unreal.load_asset(TABLE_PATH)
    if table is None:
        raise RuntimeError(f"Could not load {TABLE_PATH}")

    csv_text = unreal.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
    reader = csv.DictReader(io.StringIO(csv_text))
    field_names = reader.fieldnames
    if not field_names:
        raise RuntimeError("DT_EvidenceStates exported without a CSV header")

    required_fields = {
        "ObjectID",
        "InteractionPresentation",
        "InteractionWidgetClass",
        "bPlayWorldStoryAfterPresentation",
    }
    missing_fields = required_fields - set(field_names)
    if missing_fields:
        raise RuntimeError(f"DT_EvidenceStates is missing fields: {sorted(missing_fields)}")

    rows = list(reader)
    patched_rows = []
    for row in rows:
        if row.get("ObjectID") != DIARY_OBJECT_ID:
            continue
        row["InteractionPresentation"] = "ModalWidget"
        row["InteractionWidgetClass"] = DIARY_WIDGET_CLASS
        row["bPlayWorldStoryAfterPresentation"] = "True"
        # Every diary state must be able to resolve the same follow-up story. The closed
        # state transitions to OPEN before presentation; MEMORY remains repeatable.
        row["PhotoID"] = DIARY_PHOTO_ID
        if row.get("StateID") == "STATE_01_004_MEMORY":
            row["InteractionBehavior"] = "Repeatable"
        patched_rows.append(row.get(field_names[0], row.get("StateID", "")))

    if not patched_rows:
        raise RuntimeError(f"No states found for {DIARY_OBJECT_ID}")

    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=field_names, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)

    fill_result = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(
        table, output.getvalue()
    )
    if fill_result is False:
        raise RuntimeError("Failed to update DT_EvidenceStates from patched CSV")

    object_path = DIARY_WIDGET_CLASS.split("'", 2)[1]
    if unreal.load_class(None, object_path) is None:
        raise RuntimeError(f"Diary widget class could not be loaded: {object_path}")

    if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {TABLE_PATH}")

    unreal.log(f"Configured deferred diary story for states: {patched_rows}")


main()
