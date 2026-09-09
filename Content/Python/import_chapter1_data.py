import os
import unreal

PROJECT_ROOT = unreal.SystemLibrary.get_project_directory()
CSV_DIR = os.path.join(PROJECT_ROOT, "Scripts", "Investigation")

DATA_TABLES = [
    "DT_Characters",
    "DT_EvidenceDefinitions",
    "DT_EvidenceStates",
    "DT_KeywordChoices",
    "DT_KeywordDocuments",
    "DT_Photos",
    "DT_Sentences",
    "DT_Words",
]

AUDIO_DEST_PATH = "/Game/Balhwajeom/Audio/Voice"
AUDIO_SOURCE_FILE = os.path.normpath(
    "C:/Users/User/Downloads/SISTER_PILLOW_SOUND.mp3"
)
AUDIO_ASSET_NAME = "SISTER_PILLOW_SOUND"

ok = True

# --- 1) Import SISTER_PILLOW_SOUND.mp3 as a SoundWave, if not already imported ---
audio_package_path = AUDIO_DEST_PATH + "/" + AUDIO_ASSET_NAME
if unreal.EditorAssetLibrary.does_asset_exist(audio_package_path):
    unreal.log("Audio asset already exists, skipping import: %s" % audio_package_path)
else:
    task = unreal.AssetImportTask()
    task.filename = AUDIO_SOURCE_FILE
    task.destination_path = AUDIO_DEST_PATH
    task.destination_name = AUDIO_ASSET_NAME
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.factory = unreal.SoundFactory()

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if unreal.EditorAssetLibrary.does_asset_exist(audio_package_path):
        unreal.log("IMPORT_AUDIO Result=Success Asset=%s" % audio_package_path)
    else:
        unreal.log_error("IMPORT_AUDIO Result=Failure Asset=%s" % audio_package_path)
        ok = False

# --- 2) Reimport the 8 Investigation DataTables from Scripts/Investigation/*.csv ---
for table_name in DATA_TABLES:
    csv_path = os.path.join(CSV_DIR, table_name + ".csv")
    package_path = "/Game/Balhwajeom/Data/Investigation/" + table_name

    data_table = unreal.load_asset(package_path)
    if data_table is None:
        unreal.log_error("DT_REIMPORT %s Result=Failure Reason=AssetNotFound" % table_name)
        ok = False
        continue
    if not os.path.isfile(csv_path):
        unreal.log_error("DT_REIMPORT %s Result=Failure Reason=CsvNotFound Path=%s" % (table_name, csv_path))
        ok = False
        continue

    success = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(data_table, csv_path)
    if success:
        unreal.EditorAssetLibrary.save_loaded_asset(data_table)
        unreal.log("DT_REIMPORT %s Result=Success" % table_name)
    else:
        unreal.log_error("DT_REIMPORT %s Result=Failure Reason=FillFailed" % table_name)
        ok = False

if not ok:
    raise RuntimeError("import_chapter1_data failed, see DT_REIMPORT/IMPORT_AUDIO log lines above")

unreal.log("IMPORT_CHAPTER1_DATA Result=Success")
