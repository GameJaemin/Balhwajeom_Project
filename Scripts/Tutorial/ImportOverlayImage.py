"""Import tutorial overlay images into one shared texture folder.

    UnrealEditor-Cmd <uproject> -ExecutePythonScript="<...>/ImportOverlayImage.py" ...

Arguments are not used: the project path contains spaces, and UnrealEditor-Cmd splits
the -ExecutePythonScript value on whitespace, so a script path plus an argument cannot
be quoted reliably. Two inputs are supported instead.

1. Drop images into STAGING_FOLDER and run the script. Every image there is imported
   as ``T_<file stem>``. This is the normal workflow.
2. Set BALHWAJEOM_OVERLAY_IMAGE_SOURCE (and optionally BALHWAJEOM_OVERLAY_IMAGE_ASSET)
   to import a single file from anywhere on disk.

Only the resulting .uasset is committed; the repository tracks no raw images, and
STAGING_FOLDER is ignored by git. CreateTutorialOverlayDataTable.py refers to these
textures by asset name, so regenerating the table never needs the source files.
"""

import os
from pathlib import Path

import unreal


ASSET_FOLDER = "/Game/Balhwajeom/UI/Tutorial/Overlay"
STAGING_FOLDER = Path(unreal.Paths.project_dir()) / "Scripts" / "Tutorial" / "OverlayImages"
SOURCE_SUFFIXES = (".png", ".jpg", ".jpeg", ".tga", ".bmp")


def asset_name_for(source_path):
    stem = Path(source_path).stem
    return stem if stem.startswith("T_") else f"T_{stem}"


def collect_sources():
    single_source = os.environ.get("BALHWAJEOM_OVERLAY_IMAGE_SOURCE")
    if single_source:
        source_path = Path(single_source)
        if not source_path.is_file():
            raise RuntimeError(f"Source image not found: {source_path}")
        requested_name = os.environ.get("BALHWAJEOM_OVERLAY_IMAGE_ASSET")
        return [(source_path, requested_name or asset_name_for(source_path))]

    if not STAGING_FOLDER.is_dir():
        raise RuntimeError(f"No staging folder to import from: {STAGING_FOLDER}")

    sources = sorted(
        path
        for path in STAGING_FOLDER.iterdir()
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    )
    if not sources:
        raise RuntimeError(f"No images found in {STAGING_FOLDER}")
    return [(path, asset_name_for(path)) for path in sources]


def import_texture(source_path, asset_name):
    task = unreal.AssetImportTask()
    task.filename = str(source_path)
    task.destination_path = ASSET_FOLDER
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError(f"Could not import {source_path}")
    return task.imported_object_paths[0]


def configure_texture(object_path):
    texture = unreal.load_asset(object_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"{object_path} is not a Texture2D")

    # The UI group keeps overlay art unmipped and unstreamed, so a tutorial screen is
    # never shown blurry for its first frames.
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {object_path}")
    return texture


def main():
    for source_path, asset_name in collect_sources():
        object_path = import_texture(source_path, asset_name)
        texture = configure_texture(object_path)
        unreal.log(
            f"TUTORIAL_OVERLAY_IMAGE_IMPORT Result=Success Asset={object_path} "
            f"Size={texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
        )


main()
