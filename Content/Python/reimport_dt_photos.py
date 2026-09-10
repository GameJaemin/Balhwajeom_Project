import os
import unreal


project_root = unreal.SystemLibrary.get_project_directory()
csv_path = os.path.join(project_root, "Scripts", "Investigation", "DT_Photos.csv")
asset_path = "/Game/Balhwajeom/Data/Investigation/DT_Photos"

data_table = unreal.load_asset(asset_path)
if data_table is None:
    raise RuntimeError("DT_Photos asset was not found: %s" % asset_path)
if not os.path.isfile(csv_path):
    raise RuntimeError("DT_Photos CSV was not found: %s" % csv_path)

if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(data_table, csv_path):
    raise RuntimeError("DT_Photos CSV import failed: %s" % csv_path)

if not unreal.EditorAssetLibrary.save_loaded_asset(data_table):
    raise RuntimeError("DT_Photos save failed: %s" % asset_path)

row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(data_table)
if "PHOTO_01_013" not in [str(row_name) for row_name in row_names]:
    raise RuntimeError("DT_Photos verification failed: PHOTO_01_013 missing")

unreal.log("REIMPORT_DT_PHOTOS Result=Success CSV=%s Asset=%s" % (csv_path, asset_path))
