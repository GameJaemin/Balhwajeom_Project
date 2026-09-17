"""Patch only the three family-photo rows in the live DT_EvidenceStates asset.

The table is exported first so editor-only or locally authored fields on other rows are
preserved. Run through UnrealEditor-Cmd with -ExecutePythonScript.
"""

import csv
import io

import unreal


TABLE_PATH = "/Game/Balhwajeom/Data/Investigation/DT_EvidenceStates"
FAMILY_WIDGETS = {
    "STATE_01_001_DUST": (
        "WidgetBlueprintGeneratedClass'/Game/Balhwajeom/UI/Family/"
        "WBP_Family1.WBP_Family1_C'"
    ),
    "STATE_01_002_DUST": (
        "WidgetBlueprintGeneratedClass'/Game/Balhwajeom/UI/Family/"
        "WBP_Family2.WBP_Family2_C'"
    ),
    "STATE_01_003_DUST": (
        "WidgetBlueprintGeneratedClass'/Game/Balhwajeom/UI/Family/"
        "WBP_Family3.WBP_Family3_C'"
    ),
}


def main():
    table = unreal.load_asset(TABLE_PATH)
    if table is None:
        raise RuntimeError(f"Could not load {TABLE_PATH}")

    csv_text = unreal.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
    reader = csv.DictReader(io.StringIO(csv_text))
    field_names = reader.fieldnames
    if not field_names:
        raise RuntimeError("DT_EvidenceStates exported without a CSV header")

    rows = list(reader)
    # Unreal's exporter labels the row-name column "---" (the authored source CSV
    # uses "Name"). Use the first exported column so this remains version-agnostic.
    row_name_field = field_names[0]
    patched = set()
    for row in rows:
        row_name = row.get(row_name_field, "")
        widget_class = FAMILY_WIDGETS.get(row_name)
        if widget_class is None:
            continue
        row["InteractionPresentation"] = "ModalWidget"
        row["InteractionWidgetClass"] = widget_class
        patched.add(row_name)

    missing = set(FAMILY_WIDGETS) - patched
    if missing:
        raise RuntimeError(f"Missing family evidence rows: {sorted(missing)}")

    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=field_names, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)

    fill_result = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(
        table, output.getvalue()
    )
    # UE 5.7 exposes the C++ problem array as a success boolean in Python.
    if fill_result is False:
        raise RuntimeError("Failed to update DT_EvidenceStates from patched CSV")

    for row_name, widget_class in FAMILY_WIDGETS.items():
        object_path = widget_class.split("'", 2)[1]
        if unreal.load_class(None, object_path) is None:
            raise RuntimeError(f"{row_name} widget class could not be loaded: {object_path}")

    if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {TABLE_PATH}")

    for row_name, widget_class in FAMILY_WIDGETS.items():
        unreal.log(f"Configured {row_name} -> {widget_class}")


main()
