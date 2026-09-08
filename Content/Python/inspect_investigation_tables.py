import unreal


for name in (
    "DT_EvidenceDefinitions",
    "DT_EvidenceStates",
    "DT_Words",
    "DT_Photos",
    "DT_KeywordDocuments",
    "DT_KeywordChoices",
    "DT_Sentences",
):
    path = f"/Game/Balhwajeom/Data/Investigation/{name}.{name}"
    table = unreal.EditorAssetLibrary.load_asset(path)
    rows = unreal.DataTableFunctionLibrary.get_data_table_row_names(table) if table else []
    unreal.log(f"INVESTIGATION_TABLE {name} Rows={len(rows)} Names={','.join(str(row) for row in rows)}")
